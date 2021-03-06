/**
 * Copyright (c) 2016 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#include "sdk_config.h"
#if USBD_ENABLED
#include "nrf_drv_usbd.h"
#include "nrf.h"
#include "nordic_common.h"
#include "nrf_drv_common.h"
#include "nrf_atomic.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "app_util_platform.h"

#include <string.h>
#include <inttypes.h>

#define NRF_LOG_MODULE_NAME ""
#if NRF_USBD_DRV_LOG_ENABLED
#else //NRF_USBD_DRV_LOG_ENABLED
#define NRF_LOG_LEVEL       0
#endif //NRF_USBD_DRV_LOG_ENABLED
#include "nrf_log.h"

#ifndef NRF_DRV_USBD_EARLY_DMA_PROCESS
/* Try to process DMA request when endpoint transmission has been detected
 * and just after last EasyDMA has been processed.
 * It speeds up the transmission a little (about 10% measured)
 * with a cost of more CPU power used.
 */
#define NRF_DRV_USBD_EARLY_DMA_PROCESS 1
#endif

#ifndef NRF_DRV_USBD_PROTO1_FIX
/* Fix event system */
#define NRF_DRV_USBD_PROTO1_FIX 1
#endif

//#define NRF_DRV_USBD_PROTO1_FIX_DEBUG
#ifdef NRF_DRV_USBD_PROTO1_FIX_DEBUG
#include "nrf_log.h"
#define NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF(...) NRF_LOG_DEBUG(__VA_ARGS__)
#else
#define NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF(...) do {} while (0)
#endif

#ifndef NRF_DRV_USBD_STARTED_EV_ENABLE
#define NRF_DRV_USBD_STARTED_EV_ENABLE    1
#endif

#if NRF_DRV_USBD_PROTO1_FIX
#include "nrf_drv_systick.h"
#endif

/**
 * @defgroup nrf_usbdraw_drv_int USB Device driver internal part
 * @internal
 * @ingroup nrf_usbdraw_drv
 *
 * This part contains auxiliary internal macros, variables and functions.
 * @{
 */

/**
 * @brief Assert endpoint number validity
 *
 * Internal macro to be used during program creation in debug mode.
 * Generates assertion if endpoint number is not valid.
 *
 * @param ep Endpoint number to validity check
 */
#define USBD_ASSERT_EP_VALID(ep) ASSERT(                                         \
    ((NRF_USBD_EPIN_CHECK(ep)  && (NRF_USBD_EP_NR_GET(ep) < NRF_USBD_EPIN_CNT )) \
    ||                                                                           \
    (NRF_USBD_EPOUT_CHECK(ep) && (NRF_USBD_EP_NR_GET(ep) < NRF_USBD_EPOUT_CNT))) \
);

/**
 * @brief Lowest position of bit for IN endpoint
 *
 * The first bit position corresponding to IN endpoint.
 * @sa ep2bit bit2ep
 */
#define USBD_EPIN_BITPOS_0   0

/**
 * @brief Lowest position of bit for OUT endpoint
 *
 * The first bit position corresponding to OUT endpoint
 * @sa ep2bit bit2ep
 */
#define USBD_EPOUT_BITPOS_0  16

/**
 * @brief Input endpoint bits mask
 */
#define USBD_EPIN_BIT_MASK (0xFFFFU << USBD_EPIN_BITPOS_0)

/**
 * @brief Output endpoint bits mask
 */
#define USBD_EPOUT_BIT_MASK (0xFFFFU << USBD_EPOUT_BITPOS_0)

/**
 * @brief Auxiliary macro to change EP number into bit position
 *
 * This macro is used by @ref ep2bit function but also for statically check
 * the bitpos values integrity during compilation.
 *
 * @param[in] ep Endpoint number.
 * @return Endpoint bit position.
 */
#define USBD_EP_BITPOS(ep) ((NRF_USBD_EPIN_CHECK(ep) ? USBD_EPIN_BITPOS_0 : USBD_EPOUT_BITPOS_0) + NRF_USBD_EP_NR_GET(ep))

/* Check it the bit positions values match defined DATAEPSTATUS bit positions */
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN1)  == USBD_EPDATASTATUS_EPIN1_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN2)  == USBD_EPDATASTATUS_EPIN2_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN3)  == USBD_EPDATASTATUS_EPIN3_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN4)  == USBD_EPDATASTATUS_EPIN4_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN5)  == USBD_EPDATASTATUS_EPIN5_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN6)  == USBD_EPDATASTATUS_EPIN6_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPIN7)  == USBD_EPDATASTATUS_EPIN7_Pos );
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT1) == USBD_EPDATASTATUS_EPOUT1_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT2) == USBD_EPDATASTATUS_EPOUT2_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT3) == USBD_EPDATASTATUS_EPOUT3_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT4) == USBD_EPDATASTATUS_EPOUT4_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT5) == USBD_EPDATASTATUS_EPOUT5_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT6) == USBD_EPDATASTATUS_EPOUT6_Pos);
STATIC_ASSERT(USBD_EP_BITPOS(NRF_DRV_USBD_EPOUT7) == USBD_EPDATASTATUS_EPOUT7_Pos);


/**
 * @name Internal auxiliary definitions for SETUP packet
 *
 * Definitions used to take out the information about last SETUP packet direction
 * from @c bmRequestType.
 * @{
 */
/** The position of DIR bit in bmRequestType inside SETUP packet */
#define USBD_DRV_REQUESTTYPE_DIR_BITPOS 7
/** The mask of DIR bit in bmRequestType inside SETUP packet */
#define USBD_DRV_REQUESTTYPE_DIR_MASK   (1U << USBD_DRV_REQUESTTYPE_DIR_BITPOS)
/** The value of DIR bit for OUT direction (Host -> Device) */
#define USBD_DRV_REQUESTTYPE_DIR_OUT    (0U << USBD_DRV_REQUESTTYPE_DIR_BITPOS)
/** The value of DIR bit for IN direction (Device -> Host) */
#define USBD_DRV_REQUESTTYPE_DIR_IN     (1U << USBD_DRV_REQUESTTYPE_DIR_BITPOS)
/** @} */

/**
 * @brief Current driver state
 */
static nrf_drv_state_t m_drv_state = NRF_DRV_STATE_UNINITIALIZED;

/**
 * @brief Event handler for the library
 *
 * Event handler that would be called on events.
 *
 * @note Currently it cannot be null if any interrupt is activated.
 */
static nrf_drv_usbd_event_handler_t m_event_handler;

/**
 * @brief Direction of last received Setup transfer
 *
 * This variable is used to redirect internal setup data event
 * into selected endpoint (IN or OUT).
 */
static nrf_drv_usbd_ep_t m_last_setup_dir;

/**
 * @brief Mark endpoint readiness for DMA transfer
 *
 * Bits in this variable are cleared and set in interrupts.
 * 1 means that endpoint is ready for DMA transfer.
 * 0 means that DMA transfer cannot be performed on selected endpoint.
 */
static uint32_t m_ep_ready;

/**
 * @brief Mark endpoint with prepared data to transfer by DMA
 *
 * This variable can be from any place in the code (interrupt or main thread).
 * It would be cleared only from USBD interrupt.
 *
 * Mask prepared USBD data for transmission.
 * It is cleared when no more data to transmit left.
 */
static uint32_t m_ep_dma_waiting;

/**
 * @brief Current EasyDMA state
 *
 * Single flag, updated only inside interrupts, that marks current EasyDMA state.
 * In USBD there is only one DMA channel working in background, and new transfer
 * cannot be started when there is ongoing transfer on any other channel.
 */
static uint8_t m_dma_pending;

#if NRF_DRV_USBD_PROTO1_FIX
static uint32_t m_simulated_events;
static uint32_t m_simulated_dataepstatus;
#endif

/**
 * @brief The structure that would hold transfer configuration to every endpoint
 *
 * The structure that holds all the data required by the endpoint to proceed
 * with LIST functionality and generate quick callback directly when data
 * buffer is ready.
 */
typedef struct
{
    nrf_drv_usbd_transfer_handler_desc_t handler_desc;    //!< Handler for current transfer, function pointer and context
    nrf_drv_usbd_transfer_t              transfer_set;    //!< Last configured transfer
    nrf_drv_usbd_transfer_t              transfer_left;   //!< Current on-going transfer status
    uint16_t                             max_packet_size; //!< Configured endpoint size
    nrf_drv_usbd_ep_status_t             status;          //!< NRF_SUCCESS or error code, never NRF_ERROR_BUSY - this one is calculated
}usbd_drv_ep_state_t;

/**
 * @brief The array of transfer configurations for the endpoints.
 *
 * The status of the transfer on each endpoint.
 */
static struct
{
    usbd_drv_ep_state_t ep_out[NRF_USBD_EPOUT_CNT]; //!< Status for OUT endpoints.
    usbd_drv_ep_state_t ep_in [NRF_USBD_EPIN_CNT ]; //!< Status for IN endpoints.
}m_ep_state;

/**
 * @brief Buffer used to send data directly from FLASH
 *
 * This is internal buffer that would be used to emulate the possibility
 * to transfer data directly from FLASH.
 * We do not have to care about the source of data when calling transfer functions.
 *
 * We do not need more buffers that one, because only one transfer can be pending
 * at once.
 */
static uint32_t m_tx_buffer[CEIL_DIV(NRF_DRV_USBD_EPSIZE, sizeof(uint32_t))];


#if NRF_DRV_USBD_PROTO1_FIX
static inline nrf_usbd_event_t nrf_drv_usbd_ep_to_endevent(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);

    static const nrf_usbd_event_t epin_endev[] =
    {
        NRF_USBD_EVENT_ENDEPIN0,
        NRF_USBD_EVENT_ENDEPIN1,
        NRF_USBD_EVENT_ENDEPIN2,
        NRF_USBD_EVENT_ENDEPIN3,
        NRF_USBD_EVENT_ENDEPIN4,
        NRF_USBD_EVENT_ENDEPIN5,
        NRF_USBD_EVENT_ENDEPIN6,
        NRF_USBD_EVENT_ENDEPIN7,
        NRF_USBD_EVENT_ENDISOIN0
    };
    static const nrf_usbd_event_t epout_endev[] =
    {
        NRF_USBD_EVENT_ENDEPOUT0,
        NRF_USBD_EVENT_ENDEPOUT1,
        NRF_USBD_EVENT_ENDEPOUT2,
        NRF_USBD_EVENT_ENDEPOUT3,
        NRF_USBD_EVENT_ENDEPOUT4,
        NRF_USBD_EVENT_ENDEPOUT5,
        NRF_USBD_EVENT_ENDEPOUT6,
        NRF_USBD_EVENT_ENDEPOUT7,
        NRF_USBD_EVENT_ENDISOOUT0
    };

    return (NRF_USBD_EPIN_CHECK(ep) ? epin_endev : epout_endev)[NRF_USBD_EP_NR_GET(ep)];
}
#endif

static void usbd_dmareq_process(void);

/**
 * @brief Get interrupt mask for selected endpoint
 *
 *
 */
static inline uint32_t nrf_drv_usbd_ep_to_int(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);

    static const uint8_t epin_bitpos[] =
    {
        USBD_INTEN_ENDEPIN0_Pos,
        USBD_INTEN_ENDEPIN1_Pos,
        USBD_INTEN_ENDEPIN2_Pos,
        USBD_INTEN_ENDEPIN3_Pos,
        USBD_INTEN_ENDEPIN4_Pos,
        USBD_INTEN_ENDEPIN5_Pos,
        USBD_INTEN_ENDEPIN6_Pos,
        USBD_INTEN_ENDEPIN7_Pos,
        USBD_INTEN_ENDISOIN_Pos
    };
    static const uint8_t epout_bitpos[] =
    {
        USBD_INTEN_ENDEPOUT0_Pos,
        USBD_INTEN_ENDEPOUT1_Pos,
        USBD_INTEN_ENDEPOUT2_Pos,
        USBD_INTEN_ENDEPOUT3_Pos,
        USBD_INTEN_ENDEPOUT4_Pos,
        USBD_INTEN_ENDEPOUT5_Pos,
        USBD_INTEN_ENDEPOUT6_Pos,
        USBD_INTEN_ENDEPOUT7_Pos,
        USBD_INTEN_ENDISOOUT_Pos
    };

    return 1UL << (NRF_USBD_EPIN_CHECK(ep) ? epin_bitpos : epout_bitpos)[NRF_USBD_EP_NR_GET(ep)];
}

/**
 * @brief Change Driver endpoint number to HAL endpoint number
 *
 * @param ep Driver endpoint identifier
 *
 * @return Endpoint identifier in HAL
 *
 * @sa nrf_drv_usbd_ep_from_hal
 */
static inline uint8_t ep_to_hal(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);
    return (uint8_t)ep;
}

/**
 * @brief Generate start task number for selected endpoint index
 *
 * @param ep Endpoint number
 *
 * @return Task for starting EasyDMA transfer on selected endpoint.
 */
static inline nrf_usbd_task_t task_start_ep(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);
    return (nrf_usbd_task_t)(
        (NRF_USBD_EPIN_CHECK(ep) ? NRF_USBD_TASK_STARTEPIN0 : NRF_USBD_TASK_STARTEPOUT0) +
        (NRF_USBD_EP_NR_GET(ep) * sizeof(uint32_t)));
}

/**
 * @brief Access selected endpoint state structure
 *
 * Function used to change or just read the state of selected endpoint.
 * It is used for internal transmission state.
 *
 * @param ep Endpoint number
 */
static inline usbd_drv_ep_state_t* ep_state_access(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);
    return ((NRF_USBD_EPIN_CHECK(ep) ? m_ep_state.ep_in : m_ep_state.ep_out) + NRF_USBD_EP_NR_GET(ep));
}

/**
 * @brief Change endpoint number to bit position
 *
 * Bit positions are defined the same way as they are placed in DATAEPSTATUS register,
 * but bits for endpoint 0 are included.
 *
 * @param ep Endpoint number
 *
 * @return Bit position related to the given endpoint number
 *
 * @sa bit2ep
 */
static inline uint8_t ep2bit(nrf_drv_usbd_ep_t ep)
{
    USBD_ASSERT_EP_VALID(ep);
    return USBD_EP_BITPOS(ep);
}

/**
 * @brief Change bit position to endpoint number
 *
 * @param bitpos Bit position
 *
 * @return Endpoint number corresponding to given bit position.
 *
 * @sa ep2bit
 */
static inline nrf_drv_usbd_ep_t bit2ep(uint8_t bitpos)
{
    STATIC_ASSERT(USBD_EPOUT_BITPOS_0 > USBD_EPIN_BITPOS_0);
    return (nrf_drv_usbd_ep_t)((bitpos >= USBD_EPOUT_BITPOS_0) ? NRF_USBD_EPOUT(bitpos - USBD_EPOUT_BITPOS_0) : NRF_USBD_EPIN(bitpos));
}

/**
 * @brief Start selected EasyDMA transmission
 *
 * This is internal auxiliary function.
 * No checking is made if EasyDMA is ready for new transmission.
 *
 * @param[in] ep Number of endpoint for transmission.
 *               If it is OUT endpoint transmission would be directed from endpoint to RAM.
 *               If it is in endpoint transmission would be directed from RAM to endpoint.
 */
static inline void usbd_dma_start(nrf_drv_usbd_ep_t ep)
{
    nrf_usbd_task_trigger(task_start_ep(ep));
}

/**
 * @brief Abort pending transfer on selected endpoint
 *
 * @param ep Endpoint number.
 *
 * @note
 * This function locks interrupts that may be costly.
 * It is good idea to test if the endpoint is still busy before calling this function:
 * @code
   (m_ep_dma_waiting & (1U << ep2bit(ep)))
 * @endcode
 * This function would check it again, but it makes it inside critical section.
 */
static inline void usbd_ep_abort(nrf_drv_usbd_ep_t ep)
{
    CRITICAL_REGION_ENTER();

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);

    if(NRF_USBD_EPOUT_CHECK(ep))
    {
        /* Host -> Device */
        if((~m_ep_dma_waiting) & (1U<<ep2bit(ep)))
        {
            /* If the bit in m_ep_dma_waiting in cleared - nothing would be
             * processed inside transfer processing */
            nrf_drv_usbd_transfer_out_drop(ep);
        }
        else
        {
            p_state->transfer_left.size = 0;
            p_state->transfer_set.size  = 0;
            m_ep_dma_waiting &= ~(1U<<ep2bit(ep));
            m_ep_ready &= ~(1U<<ep2bit(ep));
        }
        /* Aborted */
        p_state->status = NRF_USBD_EP_ABORTED;
    }
    else
    {
        if((m_ep_dma_waiting | (~m_ep_ready)) & (1U<<ep2bit(ep)))
        {
            /* Device -> Host */
            m_ep_dma_waiting &= ~(1U<<ep2bit(ep));
            m_ep_ready       |=   1U<<ep2bit(ep) ;

            p_state->transfer_left.size = 0;
            p_state->transfer_set.size  = 0;
            p_state->status = NRF_USBD_EP_ABORTED;
            NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_ABORTED);
            m_event_handler(&evt);
        }
    }
    CRITICAL_REGION_EXIT();
}

void usbd_drv_ep_abort(nrf_drv_usbd_ep_t ep)
{
    usbd_ep_abort(ep);
}


/**
 * @brief Abort all pending endpoints
 *
 * Function aborts all pending endpoint transfers.
 */
static void usbd_ep_abort_all(void)
{
    uint32_t ep_waiting = m_ep_dma_waiting | (m_ep_ready & USBD_EPOUT_BIT_MASK);
    while(0 != ep_waiting)
    {
        uint8_t bitpos = __CLZ(__RBIT(ep_waiting));
        usbd_ep_abort(bit2ep(bitpos));
        ep_waiting &= ~(1U << bitpos);
    }

    m_ep_ready = (((1U<<NRF_USBD_EPIN_CNT) - 1U) << USBD_EPIN_BITPOS_0);
}

/**
 * @brief Safely call next transfer callback
 *
 * Function safely calls next transfer callback.
 *
 * @param[in,out] p_state Endpoint state structure pointer.
 *                        The callback function pointer is located here,
 *                        but also the structure with transfer information - this one might be updated.
 *
 * @retval false There is no new data or not even the next transfer callback function.
 * @retval true  There is new data to transfer (it can be also ZLP).
 */
static bool usbd_next_transfer_safe_call(usbd_drv_ep_state_t * p_state)
{
    if(NULL == p_state->handler_desc.handler)
        return false;
    bool ret;
    ret = p_state->handler_desc.handler(&(p_state->transfer_set), p_state->handler_desc.p_context);
    if(ret)
    {
        p_state->transfer_left = p_state->transfer_set;
    }
    return ret;
}

/**
 * @name Auxiliary functions for global interrupt state
 *
 * This functions are manipulating interrupt state on NVIC level.
 * In the target USBD API there would be only one interrupt.
 * So for RawIP we can only enable all interrupts or disable them.
 * @{
 */

    /**
     * Enable USBD interrupt
     */
    static inline void usbd_int_enable(void)
    {
        NVIC_EnableIRQ(USBD_IRQn);
    }

    /**
     * Disable USBD interrupt
     */
    static inline void usbd_int_disable(void)
    {
        NVIC_DisableIRQ(USBD_IRQn);
    }

    /**
     * Get current USBD interrupt state
     */
    static inline bool usbd_int_state(void)
    {
        return nrf_drv_common_irq_enable_check(USBD_IRQn);
    }

    /**
     * @brief Force the USBD interrupt into pending state
     *
     * This function is used to force USBD interrupt to be processed right now.
     * It makes it possible to process all EasyDMA access on one thread priority level.
     */
    static inline void usbd_int_rise(void)
    {
        NVIC_SetPendingIRQ(USBD_IRQn);
    }

/** @} */

/**
 * @name USBD interrupt runtimes
 *
 * Interrupt runtimes that would be vectorized using @ref m_ivec_isr
 * @{
 */

static void USBD_ISR_Usbreset(void)
{
    m_last_setup_dir = NRF_DRV_USBD_EPOUT0;
    usbd_ep_abort_all();

    const nrf_drv_usbd_evt_t evt = {
            .type = NRF_DRV_USBD_EVT_RESET
    };

    m_event_handler(&evt);
}

static void USBD_ISR_Started(void)
{
#if NRF_DRV_USBD_STARTED_EV_ENABLE
    uint32_t epstatus = nrf_usbd_epstatus_get_and_clear();

    /* All finished endpoint have to be marked as busy */
    // #warning Check this one
    // ASSERT(epstatus == ((~m_ep_ready) & epstatus));
    while (epstatus)
    {
        uint8_t           bitpos = __CLZ(__RBIT(epstatus));
        nrf_drv_usbd_ep_t ep     = bit2ep(bitpos);
        epstatus                &= ~(1UL << bitpos);

        UNUSED_VARIABLE(ep);
    }
#endif
}

/**
 * @brief Handler for EasyDMA event without endpoint clearing.
 *
 * This handler would be called when EasyDMA transfer for endpoints that does not require clearing.
 * All in endpoints are cleared automatically when new EasyDMA transfer is initialized.
 * For endpoint 0 see @ref nrf_usbd_ep0out_dma_handler
 *
 * @param[in] ep Endpoint number
 */
static inline void nrf_usbd_ep0in_dma_handler(void)
{
    NRF_LOG_DEBUG("USB event: DMA ready IN0\r\n");
    m_dma_pending = 0;

    usbd_drv_ep_state_t * p_state = ep_state_access(NRF_DRV_USBD_EPIN0);

    nrf_drv_usbd_setup_data_clear();

    if(0 == p_state->transfer_left.size)
    {
        /* Check if there is any new data from callback function */
        if(!usbd_next_transfer_safe_call(p_state))
        {
            /* End of transfer */
            UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U<<ep2bit(NRF_DRV_USBD_EPIN0))));
        }
    }
    else
    {
        // nrf_drv_usbd_setup_data_clear();
    }
}

/**
 * @brief Handler for EasyDMA event without endpoint clearing.
 *
 * This handler would be called when EasyDMA transfer for endpoints that does not require clearing.
 * All in endpoints are cleared automatically when new EasyDMA transfer is initialized.
 * For endpoint 0 see @ref nrf_usbd_ep0out_dma_handler
 *
 * @param[in] ep Endpoint number
 */
static inline void nrf_usbd_epin_dma_handler(nrf_drv_usbd_ep_t ep)
{
    NRF_LOG_DEBUG("USB event: DMA ready IN: %x\r\n", ep);
    ASSERT(NRF_USBD_EPIN_CHECK(ep));
    ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    m_dma_pending = 0;

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);

    if(0 == p_state->transfer_left.size)
    {
        /* Check if there is any new data from callback function */
        if(!usbd_next_transfer_safe_call(p_state))
        {
            /* End of transfer */
            UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U<<ep2bit(ep))));
        }
    }
}

/**
 * @brief Handler for EasyDMA event from in isochronous endpoint
 *
 * @todo RK documentation
 */
static inline void nrf_usbd_epiniso_dma_handler(nrf_drv_usbd_ep_t ep)
{
    ASSERT(NRF_USBD_EPIN_CHECK(ep));
    ASSERT(NRF_USBD_EPISO_CHECK(ep));

    m_dma_pending = 0;

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);
    if (p_state->transfer_left.size == 0)
    {
        /* Check if there is any new data from callback function */
        if(!usbd_next_transfer_safe_call(p_state))
        {
            /* End of transfer */
            UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));

            /* Send event to the user - for ISO endpoint the whole transfer is finished in this moment */
            NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OK);
            m_event_handler(&evt);
        }
    }
}

/**
 * @brief Handler for EasyDMA event for OUT endpoint 0.
 *
 * EP0 OUT have to be cleared automatically in special way - only in the middle of the transfer.
 * It cannot be cleared when required transfer is finished because it means the same that accepting the comment.
 */
static inline void nrf_usbd_ep0out_dma_handler(void)
{
    NRF_LOG_DEBUG("USB event: DMA ready OUT0\r\n");
    m_dma_pending = 0;

    usbd_drv_ep_state_t * p_state = ep_state_access(NRF_DRV_USBD_EPOUT0);

    if (p_state->transfer_left.size == 0)
    {
        /* Check if there is any new data buffer from callback function */
        if(!usbd_next_transfer_safe_call(p_state))
        {
            /* End of transfer */
            UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U<<ep2bit(NRF_DRV_USBD_EPOUT0))));
            /* Send event to the user - for OUT endpoint the whole transfer is finished in this moment */

            NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, NRF_DRV_USBD_EPOUT0, NRF_USBD_EP_OK);
            m_event_handler(&evt);
            return;
        }
    }

    nrf_drv_usbd_setup_data_clear();
}

/**
 * @brief Handler for EasyDMA event from endpoinpoint that requires clearing.
 *
 * This handler would be called when EasyDMA transfer for OUT endpoint has been finished.
 *
 * @param[in] ep Endpoint number
 *
 */
static inline void nrf_usbd_epout_dma_handler(nrf_drv_usbd_ep_t ep)
{
    NRF_LOG_DEBUG("USB drv: DMA ready OUT: %x\r\n", ep);
    ASSERT(NRF_USBD_EPOUT_CHECK(ep));
    ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    m_dma_pending = 0;

    /* Short transfer should finish */
    usbd_drv_ep_state_t * p_state = ep_state_access(ep);

    if(NRF_USBD_EP_ABORTED == p_state->status)
    {
        /* Nothing to do - just ignore */
    }
    else if (p_state->transfer_left.size == 0)
    {
        /* Check if there is any new data buffer from callback function */
        if(!usbd_next_transfer_safe_call(p_state))
        {
            NRF_LOG_DEBUG("USBD drv: epout_dma_handler ep: %02x, NRF_USBD_EP_OK\r\n", ep);
            /* End of transfer */
            UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U<<ep2bit(ep))));

            /* Send event to the user - for OUT endpoint the whole transfer is finished in this moment */
            NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OK);
            m_event_handler(&evt);
        }
    }
    else if(nrf_drv_usbd_epout_size_get(ep) < nrf_drv_usbd_ep_max_packet_size_get(ep))
    {
        /* Short transfer received - transfer finished */
        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U<<ep2bit(ep))));
        NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OK);
        m_event_handler(&evt);
    }

    nrf_usbd_epout_clear(ep);
#if NRF_DRV_USBD_EARLY_DMA_PROCESS
    /* Speed up */
    usbd_dmareq_process();
#endif
}

/**
 * @brief Handler for EasyDMA event from out isochronous endpoint
 *
 * @todo RK documentation
 */

static inline void nrf_usbd_epoutiso_dma_handler(nrf_drv_usbd_ep_t ep)
{
    ASSERT(NRF_USBD_EPISO_CHECK(ep));

    m_dma_pending = 0;

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);
    if (NRF_USBD_EP_ABORTED == p_state->status)
    {
        /* Nothing to do - just ignore */
    }
    else if (p_state->transfer_left.size == 0)
    {
        /* Check if there is any new data buffer from callback function */
        if(!usbd_next_transfer_safe_call(p_state))
        {
            /* End of transfer */
            UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U<<ep2bit(ep))));

            /* Send event to the user - for OUT endpoint the whole transfer is finished in this moment */
            NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OK);
            m_event_handler(&evt);
        }
    }
}


static void USBD_ISR_dma_epin0(void)  { nrf_usbd_ep0in_dma_handler(); }
static void USBD_ISR_dma_epin1(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN1 ); }
static void USBD_ISR_dma_epin2(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN2 ); }
static void USBD_ISR_dma_epin3(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN3 ); }
static void USBD_ISR_dma_epin4(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN4 ); }
static void USBD_ISR_dma_epin5(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN5 ); }
static void USBD_ISR_dma_epin6(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN6 ); }
static void USBD_ISR_dma_epin7(void)  { nrf_usbd_epin_dma_handler(NRF_DRV_USBD_EPIN7 ); }
static void USBD_ISR_dma_epin8(void)  { nrf_usbd_epiniso_dma_handler(NRF_DRV_USBD_EPIN8 ); }

static void USBD_ISR_dma_epout0(void) { nrf_usbd_ep0out_dma_handler(); }
static void USBD_ISR_dma_epout1(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT1); }
static void USBD_ISR_dma_epout2(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT2); }
static void USBD_ISR_dma_epout3(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT3); }
static void USBD_ISR_dma_epout4(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT4); }
static void USBD_ISR_dma_epout5(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT5); }
static void USBD_ISR_dma_epout6(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT6); }
static void USBD_ISR_dma_epout7(void) { nrf_usbd_epout_dma_handler(NRF_DRV_USBD_EPOUT7); }
static void USBD_ISR_dma_epout8(void) { nrf_usbd_epoutiso_dma_handler(NRF_DRV_USBD_EPOUT8); }

static void USBD_ISR_Sof(void)
{
    nrf_drv_usbd_evt_t evt =  {
            NRF_DRV_USBD_EVT_SOF,
            .data = { .sof = { .framecnt = nrf_usbd_framecntr_get() }}
    };

    /* Process isochronous endpoints */
    m_ep_ready |=
        (1U << ep2bit(NRF_DRV_USBD_EPIN8 )) |
        (1U << ep2bit(NRF_DRV_USBD_EPOUT8));

    m_event_handler(&evt);
}

/**
 * @brief React on data transfer finished
 *
 * Auxiliary internal function.
 * @param ep     Endpoint number
 * @param bitpos Bit position for selected endpoint number
 */
static void usbd_ep_data_handler(nrf_drv_usbd_ep_t ep, uint8_t bitpos)
{
    NRF_LOG_DEBUG("USBD event: EndpointData: %x\r\n", ep);
    /* Mark endpoint ready for next DMA access */
    m_ep_ready |= (1U<<bitpos);

    if(NRF_USBD_EPIN_CHECK(ep))
    {
        /* IN endpoint (Device -> Host) */
        if(0 == (m_ep_dma_waiting & (1U<<bitpos)))
        {
            NRF_LOG_DEBUG("USBD event: EndpointData: In finished\r\n");

            /* No more data to be send - transmission finished */
            NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OK);
            m_event_handler(&evt);
        }
    }
    else
    {
        /* OUT endpoint (Host -> Device) */
        if(0 == (m_ep_dma_waiting & (1U<<bitpos)))
        {
            NRF_LOG_DEBUG("USBD event: EndpointData: Out waiting\r\n");
            /* No buffer prepared - send event to the application */
            NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_WAITING);
            m_event_handler(&evt);
        }
    }
}

static void USBD_ISR_SetupData(void)
{
    usbd_ep_data_handler(m_last_setup_dir, ep2bit(m_last_setup_dir));
}

static void USBD_ISR_Setup(void)
{
    NRF_LOG_DEBUG("USBD event: Setup (rt:%.2x r:%.2x v:%.4x i:%.4x l:%u )\r\n",
        nrf_usbd_setup_bmrequesttype_get(),
        nrf_usbd_setup_brequest_get(),
        nrf_usbd_setup_wvalue_get(),
        nrf_usbd_setup_windex_get(),
        nrf_usbd_setup_wlength_get());
    uint8_t bmRequestType = nrf_usbd_setup_bmrequesttype_get();


    if((m_ep_dma_waiting | ((~m_ep_ready) & USBD_EPIN_BIT_MASK)) & (1U <<ep2bit(m_last_setup_dir)))
    {
        NRF_LOG_DEBUG("USBD drv: Trying to abort last transfer on EP0\r\n");
        usbd_ep_abort(m_last_setup_dir);
    }

    m_last_setup_dir = ((bmRequestType & USBD_DRV_REQUESTTYPE_DIR_MASK) == USBD_DRV_REQUESTTYPE_DIR_OUT) ?
        NRF_DRV_USBD_EPOUT0 : NRF_DRV_USBD_EPIN0;

    UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~((1U<<ep2bit(NRF_DRV_USBD_EPOUT0)) | (1U<<ep2bit(NRF_DRV_USBD_EPIN0)))));
    m_ep_ready |= 1U<<ep2bit(NRF_DRV_USBD_EPIN0);


    const nrf_drv_usbd_evt_t evt = {
            .type = NRF_DRV_USBD_EVT_SETUP
    };
    m_event_handler(&evt);
}

static void USBD_ISR_Event(void)
{
    uint32_t event = nrf_usbd_eventcause_get_and_clear();

    if(event & NRF_USBD_EVENTCAUSE_ISOOUTCRC_MASK)
    {
        /* Currently no support */
    }
    if(event & NRF_USBD_EVENTCAUSE_SUSPEND_MASK)
    {
        const nrf_drv_usbd_evt_t evt = {
                .type = NRF_DRV_USBD_EVT_SUSPEND
        };
        m_event_handler(&evt);
    }
    if(event & NRF_USBD_EVENTCAUSE_RESUME_MASK)
    {
        const nrf_drv_usbd_evt_t evt = {
                .type = NRF_DRV_USBD_EVT_RESUME
        };
        m_event_handler(&evt);
    }
}

static void USBD_ISR_EpDataStatus(void)
{
    /* Get all endpoints that have acknowledged transfer */
    uint32_t dataepstatus = nrf_usbd_epdatastatus_get_and_clear();
#if NRF_DRV_USBD_PROTO1_FIX
    dataepstatus |= m_simulated_dataepstatus;
    m_simulated_dataepstatus = 0;
#endif
    NRF_LOG_DEBUG("USBD event: EndpointEPStatus: %x\r\n", dataepstatus);

    /* All finished endpoint have to be marked as busy */
    while(dataepstatus)
    {
        uint8_t           bitpos = __CLZ(__RBIT(dataepstatus));
        nrf_drv_usbd_ep_t ep     = bit2ep(bitpos);
        dataepstatus &= ~(1UL << bitpos);

        UNUSED_RETURN_VALUE(usbd_ep_data_handler(ep, bitpos));
    }
#if NRF_DRV_USBD_EARLY_DMA_PROCESS
    /* Speed up */
    usbd_dmareq_process();
#endif
}

static void USBD_ISR_AccessFault(void)
{
    /** @todo RK Currently do nothing about it.
     *  Implement it when accessfault would be better documented */
    // ASSERT(0);
}

/**
 * @brief Function to select the endpoint to start
 *
 * Function that realizes algorithm to schedule right channel for EasyDMA transfer.
 * It gets a variable with flags for the endpoints currently requiring transfer.
 *
 * @param[in] req Bit flags for channels currently requiring transfer.
 *                Bits 0...8 used for IN endpoints.
 *                Bits 16...24 used for OUT endpoints.
 * @note
 * This function would be never called with 0 as a @c req argument.
 * @return The bit number of the endpoint that should be processed now.
 */
static uint8_t usbd_dma_scheduler_algorithm(uint32_t req)
{
    /** @todo RK This is just simple algorithm for testing and should be updated */
    return __CLZ(__RBIT(req));
}

/**
 * @brief Get the size of isochronous endpoint
 *
 * The size of isochronous endpoint is configurable.
 * This function returns the size of isochronous buffer taking into account
 * current configuration.
 *
 * @param[in] ep Endpoint number.
 *
 * @return The size of endpoint buffer.
 */
static inline size_t usbd_ep_iso_capacity(nrf_drv_usbd_ep_t ep)
{
    UNUSED_PARAMETER(ep);
    nrf_usbd_isosplit_t split = nrf_usbd_isosplit_get();
    if(NRF_USBD_ISOSPLIT_Half == split)
    {
        return NRF_DRV_USBD_ISOSIZE / 2;
    }
    return NRF_DRV_USBD_ISOSIZE;
}

/**
 * @brief Process all DMA requests
 *
 * Function that have to be called from USBD interrupt handler.
 * It have to be called when all the interrupts connected with endpoints transfer
 * and DMA transfer are already handled.
 */
static void usbd_dmareq_process(void)
{
    if(0 == m_dma_pending)
    {
        uint32_t req;
        while(0 != (req = m_ep_dma_waiting & m_ep_ready))
        {
            uint8_t pos = usbd_dma_scheduler_algorithm(req);
            nrf_drv_usbd_ep_t ep = bit2ep(pos);
            nrf_drv_usbd_transfer_t * p_transfer = &(ep_state_access(ep)->transfer_left);

            uint32_t data_addr; /* The address used for transmission */
            size_t size = p_transfer->size;

            if(NRF_USBD_EPIN_CHECK(ep))
            {
                if(NRF_USBD_EPISO_CHECK(ep))
                {
                    /* Isochronous endpoints supports only one transfer per frame
                     * and have to fit in the buffer */
                    ASSERT(size <= usbd_ep_iso_capacity(ep));
                }
                else
                {
                    /* Bulk transfers can be split to multiple transfers */
                    uint16_t max_packet_size = nrf_drv_usbd_ep_max_packet_size_get(ep);
                    if(size > max_packet_size)
                    {
                        size = max_packet_size;
                    }
                }
            }
            else
            {
                /* Host -> Device */
                if(size != 0)
                {
                    ASSERT(nrf_drv_is_in_RAM((const void*)(p_transfer->p_data.ptr)));
                    uint16_t rx_size = nrf_drv_usbd_epout_size_get(ep);
                    if(rx_size > size)
                    {
                        ep_state_access(ep)->status = NRF_USBD_EP_OVERLOAD;
                        UNUSED_RETURN_VALUE(nrf_atomic_u32_and(&m_ep_dma_waiting, ~(1U<<pos)));
                        /* Error - cannot fit endpoint data into buffer */

                        NRF_DRV_USBD_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_EP_OVERLOAD);
                        m_event_handler(&evt);
                        /* This endpoint would not be transmitted now, repeat the loop */
                        continue;
                    }
                    size = rx_size;
                }
            }

            if(0 == size)
            {
                data_addr = 0;
            }
            else if(nrf_drv_is_in_RAM((const void*)(p_transfer->p_data.ptr)))
            {
                data_addr = p_transfer->p_data.ptr;
            }
            else
            {
                /* Size of FLASH transfer is limited to the buffer size - this is also the limit for ISO endpoint */
                ASSERT(size <= sizeof(m_tx_buffer));
                /* Only IN endpoints supports FLASH transfers */
                ASSERT(NRF_USBD_EPIN_CHECK(ep));
                /* Copy data into internal buffer */
                data_addr = (uint32_t)m_tx_buffer;
                memcpy(m_tx_buffer, (const void *)(p_transfer->p_data.ptr), size);
            }

            m_dma_pending = 1;
            m_ep_ready &= ~(1U << pos);

            NRF_LOG_DEBUG("USB dma process: Starting transfer on EP: %x, size: %u\r\n", ep, size);

            nrf_usbd_ep_easydma_set(ep, data_addr, size);
#if NRF_DRV_USBD_PROTO1_FIX
            uint32_t cnt_end = (uint32_t)(-1);
            do
            {
                uint32_t cnt = (uint32_t)(-1);
                do
                {
                    nrf_usbd_event_clear(NRF_USBD_EVENT_STARTED);
                    usbd_dma_start(ep);
                    nrf_drv_systick_delay_us(2);
                    ++cnt;
                }while(!nrf_usbd_event_check(NRF_USBD_EVENT_STARTED));
                if(cnt)
                {
                    NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   DMA restarted: %u times\r\n", cnt);
                }

                nrf_drv_systick_delay_us(20);
                while(0 == (0x20 & *((volatile uint32_t *)(NRF_USBD_BASE + 0x474))))
                {
                    nrf_drv_systick_delay_us(2);
                }
                nrf_drv_systick_delay_us(1);

                ++cnt_end;
            } while(!nrf_usbd_event_check(nrf_drv_usbd_ep_to_endevent(ep)));
            if(cnt_end)
            {
                NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   DMA full restarted: %u times\r\n", cnt_end);
            }
#else
            usbd_dma_start(ep);
#endif

            /* Prepare next transfer */
            p_transfer->p_data.ptr += size;
            p_transfer->size       -= size;

            /* Transfer started - exit the loop */
            break;
        }
    }
}
/** @} */

typedef void (*nrf_drv_usbd_isr_t)(void);

/**
 * @brief USBD interrupt service runtimes
 *
 */
static const nrf_drv_usbd_isr_t m_isr[] =
{
    [USBD_INTEN_USBRESET_Pos   ] = USBD_ISR_Usbreset,
    [USBD_INTEN_STARTED_Pos    ] = USBD_ISR_Started,
    [USBD_INTEN_ENDEPIN0_Pos   ] = USBD_ISR_dma_epin0,
    [USBD_INTEN_ENDEPIN1_Pos   ] = USBD_ISR_dma_epin1,
    [USBD_INTEN_ENDEPIN2_Pos   ] = USBD_ISR_dma_epin2,
    [USBD_INTEN_ENDEPIN3_Pos   ] = USBD_ISR_dma_epin3,
    [USBD_INTEN_ENDEPIN4_Pos   ] = USBD_ISR_dma_epin4,
    [USBD_INTEN_ENDEPIN5_Pos   ] = USBD_ISR_dma_epin5,
    [USBD_INTEN_ENDEPIN6_Pos   ] = USBD_ISR_dma_epin6,
    [USBD_INTEN_ENDEPIN7_Pos   ] = USBD_ISR_dma_epin7,
    [USBD_INTEN_EP0DATADONE_Pos] = USBD_ISR_SetupData,
    [USBD_INTEN_ENDISOIN_Pos   ] = USBD_ISR_dma_epin8,
    [USBD_INTEN_ENDEPOUT0_Pos  ] = USBD_ISR_dma_epout0,
    [USBD_INTEN_ENDEPOUT1_Pos  ] = USBD_ISR_dma_epout1,
    [USBD_INTEN_ENDEPOUT2_Pos  ] = USBD_ISR_dma_epout2,
    [USBD_INTEN_ENDEPOUT3_Pos  ] = USBD_ISR_dma_epout3,
    [USBD_INTEN_ENDEPOUT4_Pos  ] = USBD_ISR_dma_epout4,
    [USBD_INTEN_ENDEPOUT5_Pos  ] = USBD_ISR_dma_epout5,
    [USBD_INTEN_ENDEPOUT6_Pos  ] = USBD_ISR_dma_epout6,
    [USBD_INTEN_ENDEPOUT7_Pos  ] = USBD_ISR_dma_epout7,
    [USBD_INTEN_ENDISOOUT_Pos  ] = USBD_ISR_dma_epout8,
    [USBD_INTEN_SOF_Pos        ] = USBD_ISR_Sof,
    [USBD_INTEN_USBEVENT_Pos   ] = USBD_ISR_Event,
    [USBD_INTEN_EP0SETUP_Pos   ] = USBD_ISR_Setup,
    [USBD_INTEN_EPDATA_Pos     ] = USBD_ISR_EpDataStatus,
    [USBD_INTEN_ACCESSFAULT_Pos] = USBD_ISR_AccessFault
};

/**
 * @name Interrupt handlers
 *
 * @{
 */
void USBD_IRQHandler(void)
{
    const uint32_t enabled = nrf_usbd_int_enable_get();
    uint32_t to_process = enabled;
    uint32_t active = 0;

    /* Check all enabled interrupts */
    while(to_process)
    {
        uint8_t event_nr = __CLZ(__RBIT(to_process));
        if(nrf_usbd_event_get_and_clear((nrf_usbd_event_t)nrf_drv_bitpos_to_event(event_nr)))
        {
            active |= 1UL << event_nr;
        }
        to_process &= ~(1UL << event_nr);
    }

#if NRF_DRV_USBD_PROTO1_FIX
    do {
        active |= enabled & m_simulated_events;
        m_simulated_events = 0;
#endif
#if NRF_DRV_USBD_PROTO1_FIX
    /* Event correcting */
    if((0 == m_dma_pending) && (0 != (active & (USBD_INTEN_SOF_Msk))))
    {
        uint8_t usbi, uoi, uii;
        /* Testing */
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7A9;
        uii = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        if(0 != uii)
        {
            uii &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        }

        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AA;
        uoi = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        if(0 != uoi)
        {
            uoi &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        }
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AB;
        usbi = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        if(0 != usbi)
        {
            usbi &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        }
        /* Processing */
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AC;
        uii &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
        if(0 != uii)
        {
            uint8_t rb;
            m_simulated_dataepstatus |= ((uint32_t)uii)<<USBD_EPIN_BITPOS_0;
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7A9;
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = uii;
            rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            UNUSED_VARIABLE(rb);
            NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   uii: 0x%.2x (0x%.2x)\r\n", uii, rb);
        }

        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AD;
        uoi &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
        if(0 != uoi)
        {
            uint8_t rb;
            m_simulated_dataepstatus |= ((uint32_t)uoi)<<USBD_EPOUT_BITPOS_0;
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AA;
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = uoi;
            rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            UNUSED_VARIABLE(rb);
            NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   uoi: 0x%.2u (0x%.2x)\r\n", uoi, rb);
        }

        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AE;
        usbi &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
        if(0 != usbi)
        {
            uint8_t rb;
            if(usbi & 0x01)
            {
                active |= USBD_INTEN_EP0SETUP_Msk;;
            }
            if(usbi & 0x10)
            {
                active |= USBD_INTEN_USBRESET_Msk;
            }
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AB;
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = usbi;
            rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            UNUSED_VARIABLE(rb);
            NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   usbi: 0x%.2u (0x%.2x)\r\n", usbi, rb);
        }
        if(0 != m_simulated_dataepstatus)
        {
            active |= enabled & NRF_USBD_INT_DATAEP_MASK;
        }
    }
#endif

    /* Process active interrupts */
    while(active)
    {
        uint8_t event_nr = __CLZ(__RBIT(active));
        #if 0
        /* Move SetupData priority after DMA priority */
        if(USBD_INTEN_EP0DATADONE_Pos == event_nr)
        {
            uint32_t active_temp = active & ~(1U<<USBD_INTEN_EP0DATADONE_Pos);
            if(0 != active_temp)
            {
                event_nr = __CLZ(__RBIT(active_temp));
                if(event_nr >= USBD_INTEN_SOF_Pos)
                {
                    /* Get back the EP0Data done event */
                    event_nr = USBD_INTEN_EP0DATADONE_Pos;
                }
            }
        }
        #endif


        m_isr[event_nr]();
        active &= ~(1UL << event_nr);
    }

    usbd_dmareq_process();
#if NRF_DRV_USBD_PROTO1_FIX
    } while(0 != (enabled & m_simulated_events));
#endif
}

/** @} */
/** @} */

ret_code_t nrf_drv_usbd_init(nrf_drv_usbd_event_handler_t const event_handler)
{
    UNUSED_VARIABLE(usbd_ep_iso_capacity);

#if NRF_DRV_USBD_PROTO1_FIX
    nrf_drv_systick_init();
#endif
    if(NULL == event_handler)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    if( m_drv_state != NRF_DRV_STATE_UNINITIALIZED)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    nrf_drv_clock_hfclk_request(NULL);
    while (!nrf_drv_clock_hfclk_is_running())
    {
        /* Just waiting */
    }

    m_event_handler = event_handler;
    m_drv_state = NRF_DRV_STATE_INITIALIZED;

    uint8_t n;
    for(n=0; n<NRF_USBD_EPIN_CNT; ++n)
    {
        nrf_drv_usbd_ep_t ep = NRF_DRV_USBD_EPIN(n);
        nrf_drv_usbd_ep_max_packet_size_set(ep, NRF_USBD_EPISO_CHECK(ep) ? (NRF_DRV_USBD_ISOSIZE / 2) : NRF_DRV_USBD_EPSIZE);
        usbd_drv_ep_state_t * p_state = ep_state_access(ep);
        p_state->status = NRF_USBD_EP_OK;
        p_state->transfer_left.size = 0;
        p_state->transfer_set.size  = 0;
    }
    for(n=0; n<NRF_USBD_EPOUT_CNT; ++n)
    {
        nrf_drv_usbd_ep_t ep = NRF_DRV_USBD_EPOUT(n);
        nrf_drv_usbd_ep_max_packet_size_set(ep, NRF_USBD_EPISO_CHECK(ep) ? (NRF_DRV_USBD_ISOSIZE / 2) : NRF_DRV_USBD_EPSIZE);
        usbd_drv_ep_state_t * p_state = ep_state_access(ep);
        p_state->status = NRF_USBD_EP_OK;
        p_state->transfer_left.size = 0;
        p_state->transfer_set.size  = 0;
    }

    return NRF_SUCCESS;
}

ret_code_t nrf_drv_usbd_uninit(void)
{
    if( m_drv_state != NRF_DRV_STATE_INITIALIZED)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    nrf_drv_clock_lfclk_release();
    m_event_handler = NULL;
    m_drv_state = NRF_DRV_STATE_UNINITIALIZED;
    return NRF_SUCCESS;
}

void nrf_drv_usbd_enable(void)
{
    ASSERT(m_drv_state == NRF_DRV_STATE_INITIALIZED);

    /* Prepare for READY event receiving */
    nrf_usbd_eventcause_clear(NRF_USBD_EVENTCAUSE_READY_MASK);
    /* Enable the peripheral */
    nrf_usbd_enable();
    /* Waiting for peripheral to enable, this should take a few us */
    while(0 == (NRF_USBD_EVENTCAUSE_READY_MASK & nrf_usbd_eventcause_get()))
    {
        /* Empty loop */
    }
    nrf_usbd_eventcause_clear(NRF_USBD_EVENTCAUSE_READY_MASK);

    nrf_usbd_isosplit_set(NRF_USBD_ISOSPLIT_Half);

    m_ep_ready = (((1U<<NRF_USBD_EPIN_CNT) - 1U) << USBD_EPIN_BITPOS_0);
    m_ep_dma_waiting = 0;
    m_dma_pending    = 0;
    m_last_setup_dir = NRF_DRV_USBD_EPOUT0;

    m_drv_state = NRF_DRV_STATE_POWERED_ON;
}

void nrf_drv_usbd_disable(void)
{
    ASSERT(m_drv_state != NRF_DRV_STATE_UNINITIALIZED);

    /* Stop just in case */
    nrf_drv_usbd_stop();

    /* Disable all parts */
    nrf_usbd_int_disable(nrf_usbd_int_enable_get());
    nrf_usbd_disable();
    m_dma_pending = 0;
    m_drv_state = NRF_DRV_STATE_INITIALIZED;
}

void nrf_drv_usbd_start(bool enable_sof)
{
    ASSERT(m_drv_state == NRF_DRV_STATE_POWERED_ON);

    uint32_t ints_to_enable =
           NRF_USBD_INT_USBRESET_MASK     |
           NRF_USBD_INT_STARTED_MASK      |
           NRF_USBD_INT_ENDEPIN0_MASK     |
           NRF_USBD_INT_EP0DATADONE_MASK  |
           NRF_USBD_INT_ENDEPOUT0_MASK    |
           NRF_USBD_INT_USBEVENT_MASK     |
           NRF_USBD_INT_EP0SETUP_MASK     |
           NRF_USBD_INT_DATAEP_MASK       |
           NRF_USBD_INT_ACCESSFAULT_MASK;

   if(enable_sof || NRF_DRV_USBD_PROTO1_FIX)
   {
       ints_to_enable |= NRF_USBD_INT_SOF_MASK;
   }

   /* Enable all required interrupts */
   nrf_usbd_int_enable(ints_to_enable);

   /* Enable interrupt globally */
   usbd_int_enable();

   /* Enable pullups */
   nrf_usbd_pullup_enable();
}

void nrf_drv_usbd_stop(void)
{
    ASSERT(m_drv_state == NRF_DRV_STATE_POWERED_ON);

    /* Abort transfers */
    usbd_ep_abort_all();

    /* Disable pullups */
    nrf_usbd_pullup_disable();

    /* Disable interrupt globally */
    usbd_int_disable();
}

bool nrf_drv_usbd_is_initialized(void)
{
    return (m_drv_state >= NRF_DRV_STATE_INITIALIZED);
}

bool nrf_drv_usbd_is_enabled(void)
{
    return (m_drv_state >= NRF_DRV_STATE_POWERED_ON);
}

bool nrf_drv_usbd_is_started(void)
{
    return (nrf_drv_usbd_is_enabled() && usbd_int_state());
}

void nrf_drv_usbd_ep_max_packet_size_set(nrf_drv_usbd_ep_t ep, uint16_t size)
{
    /* Only power of 2 size allowed */
    ASSERT((size != 0) && (size & (size - 1)) == 0);
    /* Packet size cannot be higher than maximum buffer size */
    ASSERT( ( NRF_USBD_EPISO_CHECK(ep)  && (size <= usbd_ep_iso_capacity(ep)))
           ||
           ((!NRF_USBD_EPISO_CHECK(ep)) && (size <= NRF_DRV_USBD_EPSIZE)));

    usbd_drv_ep_state_t * p_state = ep_state_access(ep);
    p_state->max_packet_size = size;
}

uint16_t nrf_drv_usbd_ep_max_packet_size_get(nrf_drv_usbd_ep_t ep)
{
    usbd_drv_ep_state_t const * p_state = ep_state_access(ep);
    return p_state->max_packet_size;
}

void nrf_drv_usbd_ep_enable(nrf_drv_usbd_ep_t ep)
{
    nrf_usbd_ep_enable(ep_to_hal(ep));
    nrf_usbd_int_enable(nrf_drv_usbd_ep_to_int(ep));

    if((NRF_USBD_EP_NR_GET(ep) != 0) && NRF_USBD_EPOUT_CHECK(ep))
    {
        CRITICAL_REGION_ENTER();

        m_ep_ready |= 1U<<ep2bit(ep);
        usbd_drv_ep_state_t * p_state = ep_state_access(ep);

        if (!NRF_USBD_EPISO_CHECK(ep))
        {
            ret_code_t ret;
            NRF_DRV_USBD_TRANSFER_OUT(transfer, 0, 0);
            ret = nrf_drv_usbd_ep_transfer(ep, &transfer, NULL);
            ASSERT(ret == NRF_SUCCESS);
            UNUSED_VARIABLE(ret);
        }
        p_state->status = NRF_USBD_EP_ABORTED;

        CRITICAL_REGION_EXIT();
    }
}

void nrf_drv_usbd_ep_disable(nrf_drv_usbd_ep_t ep)
{
    nrf_usbd_ep_disable(ep_to_hal(ep));
    nrf_usbd_int_disable(nrf_drv_usbd_ep_to_int(ep));
}

ret_code_t nrf_drv_usbd_ep_transfer(
    nrf_drv_usbd_ep_t                                  ep,
    nrf_drv_usbd_transfer_t              const * const p_transfer,
    nrf_drv_usbd_transfer_handler_desc_t const * const p_handler)
{
    uint8_t ep_bitpos = ep2bit(ep);
    ret_code_t ret;
    ASSERT(NULL != p_transfer);

    NRF_LOG_DEBUG("USB drv: Transfer called on endpoint %x, size: %u\r\n", ep, p_transfer->size);
    CRITICAL_REGION_ENTER();
    /* Setup data transaction can go only in one direction at a time */
    if((NRF_USBD_EP_NR_GET(ep) == 0) && (ep != m_last_setup_dir))
    {
        ret = NRF_ERROR_INVALID_ADDR;
    }
    else if((m_ep_dma_waiting | ((~m_ep_ready) & USBD_EPIN_BIT_MASK)) & (1U << ep_bitpos))
    {
        /* IN (Device -> Host) transfer has to be transmitted out to allow new transmission */
        ret = NRF_ERROR_BUSY;
    }
    else if(nrf_usbd_ep_is_stall(ep))
    {
        ret = NRF_ERROR_FORBIDDEN;
    }
    else
    {
        /* We can configure the transfer now */
        usbd_drv_ep_state_t * p_state =  ep_state_access(ep);
        if(NULL != p_handler)
        {
            p_state->handler_desc = *p_handler;
        }
        else
        {
            p_state->handler_desc.handler = NULL;
        }

        p_state->transfer_set = *p_transfer;
        p_state->transfer_left = *p_transfer;
        p_state->status = NRF_USBD_EP_OK;
        m_ep_dma_waiting |= 1U << ep_bitpos;

        ret = NRF_SUCCESS;
        usbd_int_rise();
    }

    CRITICAL_REGION_EXIT();
    return ret;
}

ret_code_t nrf_drv_usbd_ep_status_get(nrf_drv_usbd_ep_t ep, nrf_drv_usbd_transfer_t * p_transfer)
{
    ret_code_t ret;

    CRITICAL_REGION_ENTER();
    if(m_ep_dma_waiting & (1U << ep2bit(ep)))
    {
        ret = NRF_ERROR_BUSY;
    }
    else
    {
        usbd_drv_ep_state_t * p_state = ep_state_access(ep);
        p_transfer->p_data = p_state->transfer_set.p_data;
        p_transfer->size   = p_state->transfer_set.size - p_state->transfer_left.size;
        ret = p_state->status;
    }

    CRITICAL_REGION_EXIT();
    return ret;
}

size_t     nrf_drv_usbd_epout_size_get(nrf_drv_usbd_ep_t ep)
{
    return nrf_usbd_epout_size_get(ep_to_hal(ep));
}

bool       nrf_drv_usbd_ep_is_busy(nrf_drv_usbd_ep_t ep)
{
    return (0 != (m_ep_dma_waiting & (1UL << ep2bit(ep))));
}

void nrf_drv_usbd_ep_stall(nrf_drv_usbd_ep_t ep)
{
    NRF_LOG_DEBUG("USB: EP %x stalled.\r\n", ep);
    nrf_usbd_ep_stall(ep_to_hal(ep));
}

void nrf_drv_usbd_ep_stall_clear(nrf_drv_usbd_ep_t ep)
{
    nrf_usbd_ep_unstall(ep_to_hal(ep));
}

bool nrf_drv_usbd_ep_stall_check(nrf_drv_usbd_ep_t ep)
{
    return nrf_usbd_ep_is_stall(ep_to_hal(ep));
}

void nrf_drv_usbd_setup_get(nrf_drv_usbd_setup_t * const p_setup)
{
    memset(p_setup, 0, sizeof(nrf_drv_usbd_setup_t));
    p_setup->bmRequestType = nrf_usbd_setup_bmrequesttype_get();
    p_setup->bmRequest     = nrf_usbd_setup_brequest_get();
    p_setup->wValue        = nrf_usbd_setup_wvalue_get();
    p_setup->wIndex        = nrf_usbd_setup_windex_get();
    p_setup->wLength       = nrf_usbd_setup_wlength_get();
}

void nrf_drv_usbd_setup_data_clear(void)
{
    nrf_usbd_task_trigger(NRF_USBD_TASK_EP0RCVOUT);
}

void nrf_drv_usbd_setup_clear(void)
{
    nrf_usbd_task_trigger(NRF_USBD_TASK_EP0RCVOUT);
    nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STATUS);
}

void nrf_drv_usbd_setup_stall(void)
{
    NRF_LOG_DEBUG("Setup stalled.\r\n");
    nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STALL);
}

nrf_drv_usbd_ep_t nrf_drv_usbd_last_setup_dir_get(void)
{
    return m_last_setup_dir;
}

void nrf_drv_usbd_transfer_out_drop(nrf_drv_usbd_ep_t ep)
{
    ASSERT(NRF_USBD_EPOUT_CHECK(ep));

    if(m_ep_ready & (1U << ep2bit(ep)))
    {
        if (!NRF_USBD_EPISO_CHECK(ep))
        {
            ret_code_t ret;
            NRF_DRV_USBD_TRANSFER_OUT(transfer, 0, 0);
            ret = nrf_drv_usbd_ep_transfer(ep, &transfer, NULL);
            ASSERT(ret == NRF_SUCCESS);
            UNUSED_VARIABLE(ret);
        }
    }
}

#endif // USBD_ENABLED
