/*
 * utl_cbf.h
 *
 *  Created on: Oct 5, 2025
 *      Author: pedro-bittencourt
 */

#pragma once

typedef enum
{
    UTL_CBF_OK = 0,
    UTL_CBF_EMPTY,
    UTL_CBF_FULL,
    UTL_CBF_PARTIAL,
} utl_cbf_status_t;


typedef struct utl_cbf_s
{
    volatile size_t prod;
    volatile size_t cons;
    uint32_t size;
    uint8_t *buffer;
} utl_cbf_t;

#define UTL_CBF_DECLARE(name,_size)             \
    static uint8_t name##buffer[_size + 1];     \
    static utl_cbf_t name = {                   \
        .prod = 0,                              \
        .cons = 0,                              \
        .size = _size+1,                        \
        .buffer = (uint8_t*) name##buffer,      \
    }

/**
 @brief Retorna a quantidade de bytes disponível para consumo num buffer circular.
 @param[in] cb - ponteiro para o buffer circular.
 @return quantidade de bytes disponível para consumo
*/
uint32_t utl_cbf_bytes_available(utl_cbf_t *cb);
/**
 @brief Esvazia um buffer circular.
 @param[in] cb - ponteiro para o buffer circular.
 @return ver @ref cbf_status_s
*/
utl_cbf_status_t utl_cbf_flush(utl_cbf_t *cb);
/**
 @brief Retira um byte do buffer circular.
 @param[in] cb - ponteiro para o buffer circular.
 @param[out] c - ponteiro para o destino do dado (previamente alocado).
 @return ver @ref cbf_status_s
*/
utl_cbf_status_t utl_cbf_get(utl_cbf_t *cb, uint8_t *c);
/**
 @brief Reinicializa um buffer circular, caso seja necessário.
 A macro @ref CBF_DECLARE já faz esse papel mas essa função pode ser usada para inicialização de forma
 independente da macro.
 @param[in] cb - ponteiro para o buffer circular.
 @param[in] area - buffer previamente alocado que será usado para armazenamento do conteúdo do buffer circular.
 @param[in] size - tamanho da área de dados apontada por @p area.
 @return ver @ref cbf_status_s
*/
utl_cbf_status_t utl_cbf_init(utl_cbf_t *cb, uint8_t *area, uint32_t size);
/**
 @brief Coloca um byte no buffer circular.
 @param[in] cb - ponteiro para o buffer circular.
 @param[in] c - byte a ser adicionado ao buffer circular.
 @return ver @ref cbf_status_s
*/
utl_cbf_status_t utl_cbf_put(utl_cbf_t *cb, uint8_t c);

/**
 * @brief Coleta todos os bytes do buffer circular.
 * @param[in] cb - ponteiro para o buffer circular.
 * @param[out] dst - ponteiro para o buffer de saída
 * @param[out] out_len - número de bytes retirados
 */
utl_cbf_status_t utl_cbf_get_all(utl_cbf_t *cb, uint8_t *dst, uint32_t *out_len);

/**
 * @brief Coleta todos os bytes do buffer circular.
 * @param[in] cb - ponteiro para o buffer circular.
 * @param[out] dst - ponteiro para o buffer de saída
 * @param[in] n - número de bytes para serem lidos
 * @param[out] out_len - número de bytes retirados
 */
utl_cbf_status_t utl_cbf_get_n(utl_cbf_t *cb, uint8_t *dst, uint32_t n, uint32_t *out_len);

/**
 * @brief Coleta todos os bytes do buffer circular.
 * @param[in] cb - ponteiro para o buffer circular.
 * @param[out] src - ponteiro para o buffer de entrada
 * @param[in] n - número de bytes para serem escritos
 * @param[out] out_len - número de bytes escritos
 */
utl_cbf_status_t utl_cbf_put_n(utl_cbf_t *cb, const uint8_t *src, uint32_t n, uint32_t *out_written);



