/**
  ******************************************************************************
  * @file     syshal_gpio.c
  * @brief    System hardware abstraction layer for GPIO.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2018 Arribada</center></h2>
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <https://www.gnu.org/licenses/>.
  *
  ******************************************************************************
  */

#include "syshal_gpio.h"
#include "syshal_firmware.h"
#include "bsp.h"

#define SYSHAL_GPIO_NUMBER_EXTI   (16)

// Create a structure for holding all GPIO interrupt callbacks
typedef struct
{
    IRQn_Type irqnb;
    void (*callback)(void);
} syshal_gpio_irq_conf_t;

static syshal_gpio_irq_conf_t syshal_gpio_irq_conf[SYSHAL_GPIO_NUMBER_EXTI] =
{
    {EXTI0_1_IRQn,  NULL}, // GPIO_PIN_0
    {EXTI0_1_IRQn,  NULL}, // GPIO_PIN_1
    {EXTI2_3_IRQn,  NULL}, // GPIO_PIN_2
    {EXTI2_3_IRQn,  NULL}, // GPIO_PIN_3
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_4
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_5
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_6
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_7
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_8
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_9
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_10
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_11
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_12
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_13
    {EXTI4_15_IRQn, NULL}, // GPIO_PIN_14
    {EXTI4_15_IRQn, NULL}  // GPIO_PIN_15
};

/**
  * @brief      This function converts the GPIO_PIN_0 value to its index in the
  *             syshal_gpio_irq_conf table
  *
  * @param[in]  pin   The HAL pin name
  *
  * @return     The pin index in the syshal_gpio_irq_conf table
  */
static uint8_t syshal_gpio_get_pin_id_priv(uint16_t pin)
{
    uint8_t id = 0;

    while (pin != 0x0001)
    {
        pin = pin >> 1;
        id++;
    }

    return id;
}

/**
 * @brief      Initialise the given GPIO pin
 *
 * @param[in]  pin   The pin
 */
int syshal_gpio_init(uint32_t pin)
{

    if (GPIOA == GPIO_Inits[pin].Port)
        __HAL_RCC_GPIOA_CLK_ENABLE();

    if (GPIOB == GPIO_Inits[pin].Port)
        __HAL_RCC_GPIOB_CLK_ENABLE();

    if (GPIOC == GPIO_Inits[pin].Port)
        __HAL_RCC_GPIOC_CLK_ENABLE();

    if (GPIOD == GPIO_Inits[pin].Port)
        __HAL_RCC_GPIOD_CLK_ENABLE();

    if (GPIOE == GPIO_Inits[pin].Port)
        __HAL_RCC_GPIOE_CLK_ENABLE();

    if (GPIOF == GPIO_Inits[pin].Port)
        __HAL_RCC_GPIOF_CLK_ENABLE();

    HAL_GPIO_Init(GPIO_Inits[pin].Port, &GPIO_Inits[pin].Init);

    return SYSHAL_GPIO_NO_ERROR;
}

/**
 * @brief      De-initialise the GPIOx peripheral registers to their default reset values
 *
 * @param[in]  pin   The pin
 */
inline void syshal_gpio_term(uint32_t pin)
{
    HAL_GPIO_DeInit(GPIO_Inits[pin].Port, GPIO_Inits[pin].Init.Pin);
}

/**
 * @brief      Toggle the given GPIO output state
 *
 * @param[in]  pin   The pin
 */
inline void syshal_gpio_set_output_toggle(uint32_t pin)
{
    HAL_GPIO_TogglePin(GPIO_Inits[pin].Port, GPIO_Inits[pin].Init.Pin);
}

/**
 * @brief      Set the given GPIO pin output to low
 *
 * @param[in]  pin   The pin
 */
__RAMFUNC inline void syshal_gpio_set_output_low(uint32_t pin)
{
    HAL_GPIO_WritePin(GPIO_Inits[pin].Port, GPIO_Inits[pin].Init.Pin, GPIO_PIN_RESET);
}

/**
 * @brief      Set the given GPIO pin output to high
 *
 * @param[in]  pin   The pin
 */
__RAMFUNC inline void syshal_gpio_set_output_high(uint32_t pin)
{
    HAL_GPIO_WritePin(GPIO_Inits[pin].Port, GPIO_Inits[pin].Init.Pin, GPIO_PIN_SET);
}

/**
 * @brief      Get the given level on the specified GPIO pin
 *
 * @param[in]  pin   The pin
 *
 * @return     true for high, false for low
 */
inline bool syshal_gpio_get_input(uint32_t pin)
{
    return ( (bool) HAL_GPIO_ReadPin(GPIO_Inits[pin].Port, GPIO_Inits[pin].Init.Pin) );
}


/**
 * @brief      This function enables the interrupt on the given pin and will
 *             generate a callback event when triggered
 *
 * @param[in]  pin                The pin
 * @param[in]  callback_function  The callback function to be called on an
 *                                interrupt on the given pin
 */
void syshal_gpio_enable_interrupt(uint32_t pin, void (*callback_function)(void))
{
    uint8_t id = syshal_gpio_get_pin_id_priv(GPIO_Inits[pin].Init.Pin);

    syshal_gpio_irq_conf[id].callback = callback_function;

    // Enable and set interrupt to the lowest priority
    HAL_NVIC_SetPriority(syshal_gpio_irq_conf[id].irqnb, 0x06, 0);
    HAL_NVIC_EnableIRQ(syshal_gpio_irq_conf[id].irqnb);
}

/**
  * @brief      This function disable the interrupt on the given pin
  *
  * @param[in]  pin   The pin
  */
void syshal_gpio_disable_interrupt(uint32_t pin)
{
    uint8_t id = syshal_gpio_get_pin_id_priv(GPIO_Inits[pin].Init.Pin);

    syshal_gpio_irq_conf[id].callback = NULL;

    HAL_NVIC_DisableIRQ(syshal_gpio_irq_conf[id].irqnb);
}

/**
  * @brief      This function his called by the HAL if the IRQ is valid
  *
  * @param      GPIO_Pin  : one of the gpio pins
  */
inline void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (RESET == __HAL_GPIO_EXTI_GET_IT(GPIO_Pin))
        return;

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);

    uint8_t irq_id = syshal_gpio_get_pin_id_priv(GPIO_Pin);

    if (syshal_gpio_irq_conf[irq_id].callback)
        syshal_gpio_irq_conf[irq_id].callback();
}

/**
  * @brief      This function handles external line 0 to 1 interrupt requests
  */
void EXTI0_1_IRQHandler(void)
{
    for (uint32_t pin = GPIO_PIN_0; pin <= GPIO_PIN_1; pin = pin << 1)
        HAL_GPIO_EXTI_Callback(pin);
}

/**
  * @brief      This function handles external line 2 to 3 interrupt requests
  */
void EXTI2_3_IRQHandler(void)
{
    for (uint32_t pin = GPIO_PIN_2; pin <= GPIO_PIN_3; pin = pin << 1)
        HAL_GPIO_EXTI_Callback(pin);
}

/**
  * @brief      This function handles external line 4 to 15 interrupt requests
  */
void EXTI4_15_IRQHandler(void)
{
    for (uint32_t pin = GPIO_PIN_4; pin <= GPIO_PIN_15; pin = pin << 1)
        HAL_GPIO_EXTI_Callback(pin);
}