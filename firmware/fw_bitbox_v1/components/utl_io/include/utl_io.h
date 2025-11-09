/*
 * utl_io.h
 *
 *  Created on: Oct 5, 2025
 *      Author: pedro-bittencourt
 */

/**
@file

@defgroup IO IO
@brief Funções relacionadas a manipulação de buffers.

As funções básicas possuem uma combinação dada pelos elementos abaixo:

 utl_io_[get|put][8|16|32|64|f|d]_[f|t][b|l]_ap

Notação:

- 8|16|32|64|f|d = tamanho do dadoe (f para float, d para double)
- [f|t][b|l] = from/to big/little
- ap[r] = realiza a adição do ponteiro [reference] ao final da operação

Exemplos:

- utl_io_get16fb()
- utl_io_putf_tl()
- utl_io_get32fl_ap()
@{

*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Tamanho do ponteiro na plataforma */
#define HAL_IO_PTR_SIZE (sizeof(void *))

/** Número de elementos em um array */
#define HAL_IO_ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

/**
  @name Funções de inversão da ordem de bytes dentro de um tipo de dado (swap).
  @{
*/

/**
  Inverte um inteiro de 16 bits sem sinal.
  @param[in] usShort inteiro de 16 bits sem sinal a ser invertido
  @return inteiro de 16 bits sem sinal invertido
 */
uint16_t utl_io_swap16 (uint16_t usShort);

/**
  Inverte um inteiro de 32 bits sem sinal.
  @param[in] ulLong inteiro de 32 bits sem sinal a ser invertido
  @return inteiro de 32 bits sem sinal invertido
 */
uint32_t utl_io_swap32 (uint32_t ulLong);

/**
  Inverte um inteiro de 16 bits sem sinal dentro de um buffer.
  @param[in,out] pucPtr ponteiro para o buffer onde está o inteiro de 16 bits sem sinal a ser invertido
*/
void utl_io_swap16p(uint8_t *pucPtr);

/**
  Inverte um inteiro de 32 bits sem sinal dentro de um buffer.
  @param[in,out] pucPtr ponteiro para o buffer onde está o inteiro de 32 bits sem sinal a ser invertido
*/
void utl_io_swap32p(uint8_t *pucPtr);

/**
  Troca os bits dentro de um byte.
  @param[in,out] ucChar ponteiro para o buffer onde está o byte cujos bits serão invertidos
*/
uint8_t  utl_io_swap8b (uint8_t ucChar);
/** @} */

/**
  @name Funções dependentes de alinhamento (GET)
  @{
*/
uint8_t utl_io_get8_fl       (uint8_t  *buf); /**< Pega um uint8_t usando little endian */
uint8_t utl_io_get8_fb       (uint8_t  *buf); /**< Pega um uint8_t usando big endian */
uint8_t utl_io_get8_fl_apr   (uint8_t **buf); /**< Pega um uint8_t usando little endian e adiciona o ponteiro */
uint8_t utl_io_get8_fb_apr   (uint8_t **buf); /**< Pega um uint8_t usando big endian e adiciona o ponteiro */
#define  utl_io_get8_fl_ap(x) utl_io_get8_fl_apr(&x) /**< Macro para facilitar @ref utl_io_get8_fl_apr */
#define  utl_io_get8_fb_ap(x) utl_io_get8_fb_apr(&x) /**< Macro para facilitar @ref utl_io_get8_fb_apr */

uint16_t utl_io_get16_fl       (uint8_t  *buf); /**< Pega um uint16_t usando little endian */
uint16_t utl_io_get16_fb       (uint8_t  *buf); /**< Pega um uint16_t usando big endian */
uint16_t utl_io_get16_fl_apr   (uint8_t **buf); /**< Pega um uint16_t usando little endian e adiciona o ponteiro */
uint16_t utl_io_get16_fb_apr   (uint8_t **buf); /**< Pega um uint16_t usando big endian e adiciona o ponteiro */
#define  utl_io_get16_fl_ap(x) utl_io_get16_fl_apr(&x) /**< Macro para facilitar @ref utl_io_get16_fl_apr */
#define  utl_io_get16_fb_ap(x) utl_io_get16_fb_apr(&x) /**< Macro para facilitar @ref utl_io_get16_fb_apr */

uint32_t  utl_io_get32_fl       (uint8_t  *buf); /**< Pega um uint32_t usando little endian */
uint32_t  utl_io_get32_fb       (uint8_t  *buf); /**< Pega um uint32_t usando big endian */
uint32_t  utl_io_get32_fl_apr   (uint8_t **buf); /**< Pega um uint32_t usando little endian e adiciona o ponteiro */
uint32_t  utl_io_get32_fb_apr   (uint8_t **buf); /**< Pega um uint32_t usando big endian e adiciona o ponteiro */
#define   utl_io_get32_fl_ap(x) utl_io_get32_fl_apr(&x) /**< Macro para facilitar @ref utl_io_get32_fl_apr */
#define   utl_io_get32_fb_ap(x) utl_io_get32_fb_apr(&x) /**< Macro para facilitar @ref utl_io_get32_fb_apr */

uint64_t utl_io_get64_fl       (uint8_t  *buf); /**< Pega um uint64_t usando little endian */
uint64_t utl_io_get64_fb       (uint8_t  *buf); /**< Pega um uint64_t usando big endian */
uint64_t utl_io_get64_fl_apr   (uint8_t **buf); /**< Pega um uint64_t usando little endian e adiciona o ponteiro */
uint64_t utl_io_get64_fb_apr   (uint8_t **buf); /**< Pega um uint64_t usando big endian e adiciona o ponteiro */
#define  utl_io_get64_fl_ap(x) utl_io_get64_fl_apr(&x) /**< Macro para facilitar @ref utl_io_get64_fl_apr */
#define  utl_io_get64_fb_ap(x) utl_io_get64_fb_apr(&x) /**< Macro para facilitar @ref utl_io_get64_fb_apr */

float   utl_io_getf_fl       (uint8_t *src_ptr); /**< Pega um float usando little endian */
float   utl_io_getf_fb       (uint8_t *src_ptr); /**< Pega um float usando big endian */
float   utl_io_getf_fl_apr   (uint8_t **src_ptr); /**< Pega um float usando little endian e adiciona o ponteiro */
float   utl_io_getf_fb_apr   (uint8_t **src_ptr); /**< Pega um float usando big endian e adiciona o ponteiro */
#define utl_io_getf_fl_ap(x) utl_io_getf_fl_apr(&x) /**< Macro para facilitar @ref utl_io_getf_fl_apr */
#define utl_io_getf_fb_ap(x) utl_io_getf_fb_apr(&x) /**< Macro para facilitar @ref utl_io_getf_fb_apr */

double  utl_io_getd_fl       (uint8_t *src_ptr); /**< Pega um double usando little endian */
double  utl_io_getd_fb       (uint8_t *src_ptr); /**< Pega um double usando big endian */
double  utl_io_getd_fl_apr   (uint8_t **src_ptr); /**< Pega um double usando little endian e adiciona o ponteiro */
double  utl_io_getd_fb_apr   (uint8_t **src_ptr); /**< Pega um double usando big endian e adiciona o ponteiro */
#define utl_io_getd_fl_ap(x) utl_io_getd_fl_apr(&x) /**< Macro para facilitar @ref utl_io_getd_fl_apr */
#define utl_io_getd_fb_ap(x) utl_io_getd_fb_apr(&x) /**< Macro para facilitar @ref utl_io_getd_fb_apr */
/** @} */

/**
  @name Funções dependentes de alinhamento (PUT)
  @{
*/
void    utl_io_put8_tl         (uint8_t value, uint8_t  *buf); /**< Coloca um uint8_t usando little endian */
void    utl_io_put8_tb         (uint8_t value, uint8_t  *buf); /**< Coloca um uint8_t usando big endian */
void    utl_io_put8_tl_apr     (uint8_t value, uint8_t **buf); /**< Coloca um uint8_t usando little endian e adiciona o ponteiro */
void    utl_io_put8_tb_apr     (uint8_t value, uint8_t **buf); /**< Coloca um uint8_t usando big endian e adiciona o ponteiro */
#define utl_io_put8_tl_ap(v,x) utl_io_put8_tl_apr(v,&x) /**< Macro para facilitar @ref utl_io_put8_tl_apr */
#define utl_io_put8_tb_ap(v,x) utl_io_put8_tb_apr(v,&x) /**< Macro para facilitar @ref utl_io_put8_tb_apr */

void    utl_io_put16_tl         (uint16_t value, uint8_t  *buf); /**< Coloca um uint16_t usando little endian */
void    utl_io_put16_tb         (uint16_t value, uint8_t  *buf); /**< Coloca um uint16_t usando big endian */
void    utl_io_put16_tl_apr     (uint16_t value, uint8_t **buf); /**< Coloca um uint16_t usando little endian e adiciona o ponteiro */
void    utl_io_put16_tb_apr     (uint16_t value, uint8_t **buf); /**< Coloca um uint16_t usando big endian e adiciona o ponteiro */
#define utl_io_put16_tl_ap(v,x) utl_io_put16_tl_apr(v,&x) /**< Macro para facilitar @ref utl_io_put16_tl_apr */
#define utl_io_put16_tb_ap(v,x) utl_io_put16_tb_apr(v,&x) /**< Macro para facilitar @ref utl_io_put16_tb_apr */

void    utl_io_put32_tl         (uint32_t value, uint8_t  *buf); /**< Coloca um uint32_t usando little endian */
void    utl_io_put32_tb         (uint32_t value, uint8_t  *buf); /**< Coloca um uint32_t usando big endian */
void    utl_io_put32_tl_apr     (uint32_t value, uint8_t **buf); /**< Coloca um uint32_t usando little endian e adiciona o ponteiro */
void    utl_io_put32_tb_apr     (uint32_t value, uint8_t **buf); /**< Coloca um uint32_t usando big endian e adiciona o ponteiro */
#define utl_io_put32_tl_ap(v,x) utl_io_put32_tl_apr(v,&x) /**< Macro para facilitar @ref utl_io_put32_tl_apr */
#define utl_io_put32_tb_ap(v,x) utl_io_put32_tb_apr(v,&x) /**< Macro para facilitar @ref utl_io_put32_tb_apr */

void    utl_io_put64_tl         (uint64_t value, uint8_t  *buf); /**< Coloca um uint64_t usando little endian */
void    utl_io_put64_tb         (uint64_t value, uint8_t  *buf); /**< Coloca um uint64_t usando big endian */
void    utl_io_put64_tl_apr     (uint64_t value, uint8_t **buf); /**< Coloca um uint64_t usando little endian e adiciona o ponteiro */
void    utl_io_put64_tb_apr     (uint64_t value, uint8_t **buf); /**< Coloca um uint64_t usando big endian e adiciona o ponteiro */
#define utl_io_put64_tl_ap(v,x) utl_io_put64_tl_apr(v,&x) /**< Macro para facilitar @ref utl_io_put64_tl_apr */
#define utl_io_put64_tb_ap(v,x) utl_io_put64_tb_apr(v,&x) /**< Macro para facilitar @ref utl_io_put64_tb_apr */

void    utl_io_putf_tl          (float value, uint8_t *buf); /**< Coloca um float usando little endian */
void    utl_io_putf_tb          (float value, uint8_t *buf); /**< Coloca um float usando big endian */
void    utl_io_putf_tl_apr      (float value, uint8_t **buf); /**< Coloca um float usando little endian e adiciona o ponteiro */
void    utl_io_putf_tb_apr      (float value, uint8_t **buf); /**< Coloca um float usando big endian e adiciona o ponteiro */
#define utl_io_putf_tl_ap(v, x) utl_io_putf_tl_apr(v, &x) /**< Macro para facilitar @ref utl_io_putf_tl_apr */
#define utl_io_putf_tb_ap(v, x) utl_io_putf_tb_apr(v, &x) /**< Macro para facilitar @ref utl_io_putf_tb_apr */

void    utl_io_putd_tl          (double value, uint8_t *buf); /**< Coloca um double usando little endian */
void    utl_io_putd_tb          (double value, uint8_t *buf); /**< Coloca um double usando big endian */
void    utl_io_putd_tl_apr      (double value, uint8_t **buf); /**< Coloca um double usando little endian e adiciona o ponteiro */
void    utl_io_putd_tb_apr      (double value, uint8_t **buf); /**< Coloca um double usando big endian e adiciona o ponteiro */
#define utl_io_putd_tl_ap(v, x) utl_io_putd_tl_apr(v, &x) /**< Macro para facilitar @ref utl_io_putd_tl_apr */
#define utl_io_putd_tb_ap(v, x) utl_io_putd_tb_apr(v, &x) /**< Macro para facilitar @ref utl_io_putd_tb_apr */

void utl_io_memcpy_tl(uint8_t *dst, const uint8_t *src, uint16_t size); /**< Copia um buffer usando little endian */

/** @} */

#ifdef __cplusplus
}
#endif

/** @} */
