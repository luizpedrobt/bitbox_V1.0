#include <stdint.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_netif.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "driver/gpio.h"
#include "esp_pm.h"

#include "wifi_conn.h"
#include "esp_timer.h"
#include "embled_app.h"

#include "esp_http_server.h"
#include "app_config.h"

#include "mqtt_app.h"

#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/dns.h"

#define BUTTON_DEBOUNCE_MS 300

#define GPIO_SEL_CFG GPIO_NUM_41
#define GPIO_FUNC_CFG GPIO_NUM_42

#define NOTIFY_BTN_WIFI     0x01
#define NOTIFY_BTN_HARDWARE 0x02

static bool is_hw_portal = false;

#pragma pack(push, 1)
typedef struct 
{
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_hdr_t;
#pragma pack(pop)

#define WIFI_MAX_RETRYS 5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define PORTAL_TIMEOUT_MS   (5 * 60 * 1000) // 5 minutos

static esp_timer_handle_t portal_timer = NULL;

static const int embl_app_pins[PORT_MAX_LEDS] =
{
    GPIO_NUM_17, GPIO_NUM_18,
};

static const char *TAG = "WIFI_CONN";

static bool wifi_init_done = false;
static bool portal_running = false;

static httpd_handle_t http_server = NULL;
static esp_netif_t *wifi_ap_netif = NULL;
static esp_netif_t *sta_netif = NULL;

static SemaphoreHandle_t wifi_state_lock = NULL;

static const char *html_gpio_page =
"<!DOCTYPE html>"
"<html lang='pt-br'>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Hardware Config</title>"
"<style>"
":root { --bg: #0f172a; --card: #1e293b; --text: #f1f5f9; --primary: #10b981; --border: #334155; }"
"body { font-family: sans-serif; background: var(--bg); color: var(--text); padding: 10px; display:flex; justify-content:center; }"
".card { background: var(--card); border-radius: 12px; padding: 15px; width: 100%; max-width: 500px; border: 1px solid var(--border); box-sizing: border-box; }"
"h2 { text-align: center; border-bottom: 1px solid var(--border); padding-bottom: 10px; margin-top:0; font-size: 1.2rem; }"
/* Estilos Gerais de Linha */
".row { background: #0f172a; border-radius: 6px; padding: 8px; margin-bottom: 8px; border: 1px solid #334155; }"
".row.disabled { opacity: 0.3; pointer-events: none; }"
/* Layout Flex */
".flex-line { display: flex; gap: 5px; align-items: center; margin-bottom: 4px; }"
".flex-line:last-child { margin-bottom: 0; }"
/* Labels e Inputs */
"label { font-size: 0.7rem; color: #cbd5f5; font-weight: bold; margin-right: 4px; }"
"select, input { flex: 1; padding: 6px; background: #1e293b; color: white; border: 1px solid var(--border); border-radius: 4px; font-size: 0.8rem; min-width: 0; }"
"button { width: 100%; padding: 12px; background: var(--primary); color: white; border: none; border-radius: 8px; font-weight: bold; cursor: pointer; margin-top: 15px; }"
".sec-title { margin: 20px 0 8px 0; color: var(--primary); font-size: 0.9rem; text-transform: uppercase; font-weight:bold; border-bottom: 1px solid #334155; }"
"input[type='checkbox'] { width: 18px; height: 18px; flex: 0 0 18px; cursor: pointer; accent-color: var(--primary); }"
".grp { display: flex; flex-direction: column; flex: 1; }" /* Agrupa Label + Input verticalmente */
".grp-l { margin-bottom: 2px; }"
"</style>"
"</head>"
"<body>"
"<div class='card'>"
"<h2>Hardware Config</h2>"
"<form method='POST' action='/save_gpio'>"

"<div class='sec-title'>UART Configuration</div>"
"<div id='u_cont'></div>"

"<div class='sec-title'>GPIO Configuration</div>"
"<div id='g_cont'></div>"

"<button type='submit'>Salvar Configuração</button>"
"</form>"
"</div>"

"<script>"
"function toggle(id, type) {"
"  const cb = document.getElementById(type + '_en_' + id);"
"  const row = document.getElementById(type + '_row_' + id);"
"  if(cb.checked) row.classList.remove('disabled');"
"  else row.classList.add('disabled');"
"}"

/* GERAÇÃO UART */
"const u_cont = document.getElementById('u_cont');"
"for(let i=0; i<3; i++) {"
"  let h = `<div style='display:flex; align-items:center; margin-bottom:2px; margin-top:8px;'>`;"
"  h += `<input type='checkbox' name='u_en_${i}' id='u_en_${i}' onchange='toggle(${i}, \"u\")'>`;"
"  h += `<label style='margin-left:8px; color:white; font-size:0.9rem;'>UART ${i}</label></div>`;"
"  h += `<div class='row disabled' id='u_row_${i}'>`;"
"  h += `<div class='flex-line'>`;"
"  h += `<div class='grp' style='flex:0.6'><span class='grp-l label'>Baud</span><input type='number' name='u_baud_${i}' value='115200'></div>`;"
"  h += `<div class='grp'><span class='grp-l label'>TX Pin</span><input type='number' name='u_tx_${i}' placeholder='IO'></div>`;"
"  h += `<div class='grp'><span class='grp-l label'>RX Pin</span><input type='number' name='u_rx_${i}' placeholder='IO'></div>`;"
"  h += `</div></div>`;"
"  u_cont.innerHTML += h;"
"}"

/* GERAÇÃO GPIO */
"const p = ['BOARD_1','BOARD_2','BOARD_3','BOARD_4','BOARD_5','BOARD_33','BOARD_34','BOARD_35','BOARD_36','BOARD_37'];"
"const g_cont = document.getElementById('g_cont');"
"p.forEach((n, i) => {"
"  let h = `<div style='display:flex; align-items:center; margin-bottom:2px; margin-top:10px;'>`;"
"  h += `<input type='checkbox' name='g_en_${i}' id='g_en_${i}' onchange='toggle(${i}, \"g\")'>`;"
"  h += `<label style='margin-left:8px; color:white; font-size:0.9rem;'>${n}</label></div>`;"
"  h += `<div class='row disabled' id='g_row_${i}'>`;"
   /* Linha 1: Mode e Interrupt */
"  h += `<div class='flex-line'>`;"
"  h += `<div class='grp'><span class='grp-l label'>Mode</span><select name='m_${i}'>`;"
"  h += `<option value='0'>DISABLE</option><option value='1'>INPUT</option><option value='2'>OUTPUT</option><option value='3'>IN_OUT</option></select></div>`;"
"  h += `<div class='grp'><span class='grp-l label'>Intr</span><select name='int_${i}'>`;"
"  h += `<option value='0'>DISABLE</option><option value='1'>POSEDGE</option><option value='2'>NEGEDGE</option><option value='3'>ANYEDGE</option><option value='4'>LOW</option><option value='5'>HIGH</option></select></div>`;"
"  h += `</div>`;"
   /* Linha 2: Pull Up e Pull Down */
"  h += `<div class='flex-line'>`;"
"  h += `<div class='grp'><span class='grp-l label'>Pull Up</span><select name='pu_${i}'><option value='0'>DISABLE</option><option value='1'>ENABLE</option></select></div>`;"
"  h += `<div class='grp'><span class='grp-l label'>Pull Down</span><select name='pd_${i}'><option value='0'>DISABLE</option><option value='1'>ENABLE</option></select></div>`;"
"  h += `</div></div>`;"
"  g_cont.innerHTML += h;"
"});"
"</script>"
"</body>"
"</html>";

static const char *html_page =
"<!DOCTYPE html>"
"<html lang='pt-br'>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Datalogger Setup</title>"
"<style>"
"body { font-family: Arial, sans-serif; background: #0f172a; color: #e5e7eb; display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; }"
".card { background: #020617; border-radius: 14px; padding: 26px 28px; width: 100%; max-width: 400px; box-shadow: 0 10px 30px rgba(0,0,0,0.6); }"
"h2 { margin: 0 0 18px 0; text-align: center; font-weight: 600; }"
"label { font-size: 0.85rem; color: #cbd5f5; }"
"input, select { width: 100%; padding: 11px 12px; margin-top: 6px; margin-bottom: 14px; border-radius: 8px; border: 1px solid #1e293b; background: #020617; color: #e5e7eb; box-sizing: border-box; }"
"input:focus, select:focus { outline: none; border-color: #38bdf8; }"
/* Checkbox Customizado */
".chk-container { display: flex; align-items: center; margin-bottom: 15px; background: #1e293b; padding: 10px; border-radius: 8px; border: 1px solid #334155; }"
"input[type='checkbox'] { width: 20px; height: 20px; margin: 0 10px 0 0; cursor: pointer; accent-color: #10b981; }"
".chk-label { font-size: 0.9rem; font-weight: bold; color: white; cursor: pointer; }"
"button { width: 100%; padding: 12px; border: none; border-radius: 10px; background: #0284c7; color: white; font-size: 1rem; font-weight: 600; cursor: pointer; }"
".disabled-area { opacity: 0.3; pointer-events: none; transition: opacity 0.3s; }"
".footer { margin-top: 14px; font-size: 0.75rem; color: #64748b; text-align: center; }"
".hidden { display: none; }"
".spinner { width: 42px; height: 42px; border: 4px solid #1e293b; border-top: 4px solid #38bdf8; border-radius: 50%; animation: spin 1s linear infinite; margin: 20px auto; }"
"@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }"
"</style>"
"</head>"
"<body>"
"<div class='card' id='formCard'>"
"<h2>Datalogger Setup</h2>"
"<form method='POST' action='/save' onsubmit='showLoading()'>"

"<div class='chk-container'>"
"<input type='checkbox' name='offline' id='offline' value='1' onchange='toggleWifi()'>"
"<label for='offline' class='chk-label'>Modo Offline (Sem Wi-Fi)</label>"
"</div>"

"<div id='wifi-area'>"
"<label>SSID</label>"
"<select name='ssid' id='ssid'>"
"<option value='' disabled selected>Carregando redes...</option>"
"</select>"
"<button type='button' onclick='loadNetworks()' style='margin-bottom:14px; background:#1e293b; font-size:0.8rem; padding:8px;'>↻ Atualizar Lista</button>"
"<label>Senha</label>"
"<input name='pass' type='password' placeholder='Senha do Wi-Fi'>"
"</div>"

"<button type='submit'>Salvar Configuração</button>"
"</form>"
"<div class='footer'>Firmware Config Portal</div>"
"</div>"

"<div class='card hidden' id='loadingCard'>"
"<h2>Configurando...</h2>"
"<div class='spinner'></div>"
"<p style='text-align:center;font-size:0.9rem;color:#cbd5f5;'>Salvando e reiniciando...</p>"
"</div>"

"<script>"
"function showLoading() { document.getElementById('formCard').classList.add('hidden'); document.getElementById('loadingCard').classList.remove('hidden'); }"
"function toggleWifi() {"
"  var chk = document.getElementById('offline');"
"  var area = document.getElementById('wifi-area');"
"  if(chk.checked) area.classList.add('disabled-area');"
"  else area.classList.remove('disabled-area');"
"}"
"function loadNetworks() {"
"  var s = document.getElementById('ssid');"
"  s.innerHTML = '<option disabled selected>Escaneando...</option>';"
"  fetch('/scan').then(r => r.json()).then(data => {"
"    s.innerHTML = '';"
"    if(data.length === 0) s.innerHTML = '<option disabled>Nenhuma rede encontrada</option>';"
"    else data.forEach(net => {"
"      var opt = document.createElement('option');"
"      opt.value = net.s; opt.innerText = net.s + ' (' + net.r + 'dBm)';"
"      s.appendChild(opt);"
"    });"
"  }).catch(e => { s.innerHTML = '<option disabled>Erro ao escanear</option>'; });"
"}"
"window.onload = loadNetworks;"
"</script>"
"</body>"
"</html>";

static EventGroupHandle_t wifi_event_group;
static int retry_num = 0;

static TaskHandle_t config_task_handle = NULL;

static struct udp_pcb *dns_pcb;

static const char *portal_uris[] = 
{
    "/",
    "/hotspot-detect.html",          // Apple
    "/library/test/success.html",    // Apple
    "/generate_204",                 // Android
    "/connecttest.txt",              // Windows
};

static esp_err_t captive_get_handler(httpd_req_t *req);
static bool wifi_conn_init_sta(wifi_config_t *cfg);
static esp_err_t save_post_handler(httpd_req_t *req);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void http_server_start(void);
static void dns_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static void wifi_dns_start(void);
static void wifi_dns_stop(void);
static void wifi_init_ap(void);
static void url_decode_inplace(char *str);
static void config_button_init(void);
static void config_button_isr(void *arg);
static void portal_timeout_cb(void *arg);

static void config_button_init(void)
{
    gpio_config_t btn_cfg = 
    {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE, 
    };

    // botão de setup do wifi
    btn_cfg.pin_bit_mask = (1ULL << GPIO_SEL_CFG);
    gpio_config(&btn_cfg);
    gpio_isr_handler_add(GPIO_SEL_CFG, config_button_isr, (void*)NOTIFY_BTN_WIFI);

    // botão de setup do hw
    btn_cfg.pin_bit_mask = (1ULL << GPIO_FUNC_CFG);
    gpio_config(&btn_cfg);
    gpio_isr_handler_add(GPIO_FUNC_CFG, config_button_isr, (void*)NOTIFY_BTN_HARDWARE);
}

static void portal_timeout_cb(void *arg)
{
    ESP_LOGW(TAG, "Timeout do captive portal");
    esp_restart();
}

static void start_portal_timeout(void)
{
    esp_err_t err;

    if (portal_timer == NULL)
    {
        const esp_timer_create_args_t timer_args =
        {
            .callback = &portal_timeout_cb,
            .name = "portal_timeout"
        };

        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &portal_timer));
    }

    err = esp_timer_stop(portal_timer);

    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Erro ao parar timer: %s", esp_err_to_name(err));
    }

    ESP_ERROR_CHECK(esp_timer_start_once(portal_timer, PORTAL_TIMEOUT_MS * 1000));
}

static void IRAM_ATTR config_button_isr(void *arg)
{
    uint32_t ntf_val = (uint32_t)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(config_task_handle)
    {
        xTaskNotifyFromISR(config_task_handle, ntf_val, eSetBits, &xHigherPriorityTaskWoken);
    }

    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

static void config_task(void *arg)
{
    static TickType_t last_button_tick = 0;
    uint32_t notify;

    while (1) 
    {
        xTaskNotifyWait(0, UINT32_MAX, &notify, portMAX_DELAY);

        TickType_t now = xTaskGetTickCount();

        if ((now - last_button_tick) < pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) 
        {
            continue;
        }
        last_button_tick = now;

        bool request_hw_portal = (notify & NOTIFY_BTN_HARDWARE) ? true : false;
        ESP_LOGI(TAG, "Solicitação de Portal via Botão (%s)", request_hw_portal ? "Hardware" : "WiFi");

        if(wifi_state_lock && xSemaphoreTake(wifi_state_lock, portMAX_DELAY))
        {
            if(!portal_running)
            {
                is_hw_portal = request_hw_portal;

                xSemaphoreGive(wifi_state_lock);
                wifi_start_captive_portal();
            }
        }

        else
        {
            ESP_LOGI(TAG, "Portal já ativo. Renovando timer e atualizando contexto.");

            if(is_hw_portal != request_hw_portal)
            {
                is_hw_portal = request_hw_portal;
                ESP_LOGI(TAG, "Contexto do portal alterado para: %s", is_hw_portal ? "Hardware" : "WiFi");
            }

            start_portal_timeout();

            xSemaphoreGive(wifi_state_lock);
        }
    }
}

static void url_decode_inplace(char *str) 
{
    char *src = str;
    char *dst = str;
    char a, b;

    while (*src) 
    {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit((int)a) && isxdigit((int)b))) 
        {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            
            *dst++ = 16 * a + b;
            src += 3;
        }

        else if (*src == '+') 
        {
            *dst++ = ' ';
            src++;
        } 

        else 
        {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
}

static esp_err_t captive_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");

    if(is_hw_portal)
    {
        httpd_resp_send(req, html_gpio_page, HTTPD_RESP_USE_STRLEN);
    }

    else
    {
        httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    }

    return ESP_OK;
}

static esp_err_t save_gpio_handler(httpd_req_t *req)
{
    if (!is_hw_portal)
    {
        ESP_LOGW(TAG, "Tentativa de salvar GPIO no portal errado.");
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Portal Invalido");
        return ESP_FAIL;
    }

    char buf[4096]; 
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) 
    {
        ESP_LOGE(TAG, "Payload muito grande");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    // Inicializa as estruturas com ZERO para evitar lixo de memória
    sys_config_uart_t sys_uart;
    sys_config_gpio_t sys_gpio;
    
    memset(&sys_uart, 0, sizeof(sys_config_uart_t));
    memset(&sys_gpio, 0, sizeof(sys_config_gpio_t));

    char val[16];
    char key[32];
    
    // --- UART ---
    int active_uart_count = 0;
    for(int i = 0; i < UART_NUM_MAX; i++)
    {
        snprintf(key, sizeof(key), "u_en_%d", i);
        if (httpd_query_key_value(buf, key, val, sizeof(val)) == ESP_OK) 
        {
            sys_uart.uarts[active_uart_count].uart_num = i;
            sys_uart.uarts[active_uart_count].state = true; 
            
            snprintf(key, sizeof(key), "u_baud_%d", i);
            if (httpd_query_key_value(buf, key, val, sizeof(val)) == ESP_OK) 
                sys_uart.uarts[active_uart_count].baudrate = atoi(val);

            snprintf(key, sizeof(key), "u_tx_%d", i);
            if (httpd_query_key_value(buf, key, val, sizeof(val)) == ESP_OK) 
                sys_uart.uarts[active_uart_count].tx_pin = atoi(val);

            snprintf(key, sizeof(key), "u_rx_%d", i);
            if (httpd_query_key_value(buf, key, val, sizeof(val)) == ESP_OK) 
                sys_uart.uarts[active_uart_count].rx_pin = atoi(val);

            active_uart_count++;
        }
    }
    sys_uart.uart_cnt = active_uart_count;

    // --- GPIO ---
    int active_gpio_count = 0;

    // Lista de mapeamento: Índice do Loop HTML -> Enum da Placa
    const int valid_gpios[] = { 
        GPIO_BOARD_1, GPIO_BOARD_2, GPIO_BOARD_3, GPIO_BOARD_4, GPIO_BOARD_5, 
        GPIO_BOARD_33, GPIO_BOARD_34, GPIO_BOARD_35, GPIO_BOARD_36, GPIO_BOARD_37 
    };
    const int num_valid_gpios = sizeof(valid_gpios)/sizeof(valid_gpios[0]);

    for (int i = 0; i < num_valid_gpios; i++) 
    {
        // Proteção para não estourar o array da struct
        if (active_gpio_count >= GPIO_BOARD_MAX) break;

        snprintf(key, sizeof(key), "g_en_%d", i);
        
        if (httpd_query_key_value(buf, key, val, sizeof(val)) == ESP_OK) 
        {
            // Debug: Verifique se isso aparece mais vezes do que deveria no log
            ESP_LOGI(TAG, "Encontrado GPIO habilitado no HTML index: %d", i);

            // Mapeia o índice do HTML (0..9) para o Enum interno
            sys_gpio.gpios[active_gpio_count].gpio_num = (gpio_available_ports_t)i; 
            sys_gpio.gpios[active_gpio_count].state = true;

            snprintf(key, sizeof(key), "m_%d", i);
            if (httpd_query_key_value(buf, key, val, sizeof(val)) == ESP_OK) 
            {
                int m = atoi(val);
                switch(m) {
                    case 1: sys_gpio.gpios[active_gpio_count].mode = GPIO_MODE_INPUT; break;
                    case 2: sys_gpio.gpios[active_gpio_count].mode = GPIO_MODE_OUTPUT; break;
                    case 3: sys_gpio.gpios[active_gpio_count].mode = GPIO_MODE_INPUT_OUTPUT; break;
                    default: sys_gpio.gpios[active_gpio_count].mode = GPIO_MODE_DISABLE; break;
                }
            }

            snprintf(key, sizeof(key), "int_%d", i);
            if (httpd_query_key_value(buf, key, val, sizeof(val)) == ESP_OK)
            {
                int intr = atoi(val);
                switch(intr) {
                    case 1: sys_gpio.gpios[active_gpio_count].intr_type = GPIO_INTR_POSEDGE; break;
                    case 2: sys_gpio.gpios[active_gpio_count].intr_type = GPIO_INTR_NEGEDGE; break;
                    case 3: sys_gpio.gpios[active_gpio_count].intr_type = GPIO_INTR_ANYEDGE; break;
                    case 4: sys_gpio.gpios[active_gpio_count].intr_type = GPIO_INTR_LOW_LEVEL; break;
                    case 5: sys_gpio.gpios[active_gpio_count].intr_type = GPIO_INTR_HIGH_LEVEL; break;
                    default: sys_gpio.gpios[active_gpio_count].intr_type = GPIO_INTR_DISABLE; break;
                }
            }

            snprintf(key, sizeof(key), "pu_%d", i);
            if (httpd_query_key_value(buf, key, val, sizeof(val)) == ESP_OK)
                sys_gpio.gpios[active_gpio_count].pull_up_en = (atoi(val) == 1) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;

            snprintf(key, sizeof(key), "pd_%d", i);
            if (httpd_query_key_value(buf, key, val, sizeof(val)) == ESP_OK)
                sys_gpio.gpios[active_gpio_count].pull_down_en = (atoi(val) == 1) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;

            active_gpio_count++;
        }
    }
    
    sys_gpio.gpio_cnt = active_gpio_count;
    ESP_LOGI(TAG, "Salvando %d GPIOs e %d UARTs", active_gpio_count, active_uart_count);

    esp_err_t err_u = app_config_uart_save(&sys_uart);
    esp_err_t err_g = app_config_gpio_save(&sys_gpio);

    if (err_u == ESP_OK && err_g == ESP_OK) 
    {
        httpd_resp_sendstr(req, "Configuração salva. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }
    else 
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t wifi_scan_handler(httpd_req_t *req)
{
    #define MAX_SCAN_RECORDS 10
    wifi_ap_record_t ap_info[MAX_SCAN_RECORDS];
    uint16_t number = MAX_SCAN_RECORDS;
    
    wifi_scan_config_t scan_config = 
    {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false
    };

    esp_wifi_scan_start(&scan_config, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    
    char *json_response = malloc(1024);
    if (!json_response) 
    {
        return ESP_FAIL;
    }

    strcpy(json_response, "[");

    for (int i = 0; i < number; i++) 
    {
        char entry[64];

        if (strlen((char *)ap_info[i].ssid) > 0) 
        {
            snprintf(entry, sizeof(entry), "{\"s\":\"%s\",\"r\":%d}%s", ap_info[i].ssid, ap_info[i].rssi, (i < number - 1) ? "," : "");
            strcat(json_response, entry);
        }
    }

    if (json_response[strlen(json_response)-1] == ',') 
    {
        json_response[strlen(json_response)-1] = '\0';
    }

    strcat(json_response, "]");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));
    
    free(json_response);
    return ESP_OK;
}

static bool wifi_conn_init_sta(wifi_config_t *cfg)
{
    wifi_event_group  = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    bool result = false;

    if (bits & WIFI_CONNECTED_BIT) 
    {
        ESP_LOGI(TAG, "Conectado à rede: %s", cfg->sta.ssid);
        wifi_init_done = true;
        result = true;
    }
        
    else if (bits & WIFI_FAIL_BIT) 
    {
        ESP_LOGW(TAG, "Falha ao conectar à rede: %s", cfg->sta.ssid);
        result = false;
    }

    return result;
}

static esp_err_t save_post_handler(httpd_req_t *req)
{
    // 2. Intertravamento: Se for portal de Hardware, rejeita salvamento de Wi-Fi
    if (is_hw_portal)
    {
        ESP_LOGW(TAG, "Tentativa de salvar Wi-Fi no portal de Hardware.");
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Portal Invalido");
        return ESP_FAIL;
    }

    char buf[224];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) return ESP_FAIL;
    buf[len] = '\0';

    sys_config_netw_t netw_cfg = { 0 };
    char val_off[4];

    if (httpd_query_key_value(buf, "offline", val_off, sizeof(val_off)) == ESP_OK)
    {
        netw_cfg.offline_mode = (atoi(val_off) == 1);
    }
    else
    {
        netw_cfg.offline_mode = false;
    }

    if (!netw_cfg.offline_mode)
    {
        httpd_query_key_value(buf, "ssid", netw_cfg.ssid, sizeof(netw_cfg.ssid));
        httpd_query_key_value(buf, "pass", netw_cfg.pass, sizeof(netw_cfg.pass));
        url_decode_inplace(netw_cfg.ssid);
        url_decode_inplace(netw_cfg.pass);
    }

    app_config_netw_save(&netw_cfg);

    httpd_resp_sendstr(req, "Configuração salva. Reiniciando...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if(portal_running)
    {
        return;
    }

    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
        
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if(retry_num <= WIFI_MAX_RETRYS)
        {
            esp_wifi_connect();
            embled_set_mode(embl_app_pins[PORT_STATUS], EMBLED_DRIVER_MODE_DIGITAL, EMBLED_MODE_BLINK_SLOW, EMBLED_ACTIVE_HIGH, false);
            retry_num++;
            ESP_LOGI(TAG, "Tentando se conectar no WiFi");
        }

        else if(retry_num > WIFI_MAX_RETRYS)
        {
            ESP_LOGW(TAG, "Falha na Conexão Wi-Fi: Máximo de tentativas atingido");
        }

        else
        {
            if(wifi_event_group != NULL)
            {
                wifi_event_sta_disconnected_t *disc_event = (wifi_event_sta_disconnected_t *) event_data;

                ESP_LOGW(TAG, "Wi-Fi Desconectado. Reason Code: %d", disc_event->reason);

                xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);

                embled_set_mode(embl_app_pins[PORT_STATUS], EMBLED_DRIVER_MODE_DIGITAL, EMBLED_MODE_BLINK_SLOW, EMBLED_ACTIVE_HIGH, false);
                mqtt_deinit_app();
            }
        }
    }
    
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Conectado com IP: "IPSTR, IP2STR(&event->ip_info.ip));
        embled_set_mode(embl_app_pins[PORT_STATUS], EMBLED_DRIVER_MODE_DIGITAL, EMBLED_MODE_ON, EMBLED_ACTIVE_HIGH, false);
        retry_num = 0;

        if(wifi_event_group != NULL)
        {
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}   

static void http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 10;

    if(http_server)
    {
        httpd_stop(http_server);
        http_server = NULL;
    }

    httpd_start(&http_server, &config);

    for (int i = 0; i < sizeof(portal_uris)/sizeof(portal_uris[0]); i++)
    {
        httpd_uri_t uri = 
        {
            .uri = portal_uris[i],
            .method = HTTP_GET,
            .handler = captive_get_handler
        };
        httpd_register_uri_handler(http_server, &uri);
    }

    httpd_uri_t save = 
    {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = save_post_handler
    };
    httpd_register_uri_handler(http_server, &save);

    httpd_uri_t scan =
    {
        .uri = "/scan",
        .method = HTTP_GET,
        .handler = wifi_scan_handler,
    };
    httpd_register_uri_handler(http_server, &scan);

    httpd_uri_t save_gpio = 
    { 
        .uri = "/save_gpio", 
        .method = HTTP_POST, 
        .handler = save_gpio_handler 
    };
    httpd_register_uri_handler(http_server, &save_gpio);
}

static void dns_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) 
{
    if (!p)
    {
        return;
    } 

    uint8_t *data = (uint8_t *)p->payload;

    if (p->tot_len < sizeof(dns_hdr_t)) 
    {
        pbuf_free(p);
        return;
    }

    uint16_t idx = sizeof(dns_hdr_t);
    
    while (idx < p->tot_len && data[idx] != 0) 
    {
        idx += data[idx] + 1;
    }
    
    idx++; 
    idx += 4; 

    if (idx > p->tot_len) 
    {
        pbuf_free(p);
        return;
    }

    struct pbuf *resp = pbuf_alloc(PBUF_TRANSPORT, idx + 16, PBUF_RAM);
    if (!resp) 
    {
        pbuf_free(p);
        return;
    }

    memcpy(resp->payload, data, idx);

    dns_hdr_t *hdr = (dns_hdr_t *)resp->payload;
    hdr->flags = htons(0x8180); 
    hdr->ancount = htons(1);    

    uint8_t *resp_data = (uint8_t *)resp->payload;
    
    resp_data[idx++] = 0xC0; resp_data[idx++] = 0x0C; 
    resp_data[idx++] = 0x00; resp_data[idx++] = 0x01; 
    resp_data[idx++] = 0x00; resp_data[idx++] = 0x01; 
    resp_data[idx++] = 0x00; resp_data[idx++] = 0x00;
    resp_data[idx++] = 0x00; resp_data[idx++] = 0x3C; 
    resp_data[idx++] = 0x00; resp_data[idx++] = 0x04; 
    
    resp_data[idx++] = 192;  
    resp_data[idx++] = 168;
    resp_data[idx++] = 4;    
    resp_data[idx++] = 1;

    resp->len = idx;
    resp->tot_len = idx;

    udp_sendto(pcb, resp, addr, port);

    pbuf_free(resp);
    pbuf_free(p);
}

static void wifi_dns_start(void)
{
    dns_pcb = udp_new();
    udp_bind(dns_pcb, IP_ADDR_ANY, 53);
    udp_recv(dns_pcb, dns_recv_cb, NULL);
}

static void wifi_dns_stop(void)
{
    if(dns_pcb)
    {
        udp_remove(dns_pcb);
        dns_pcb = NULL;
    }
}

static void wifi_init_ap(void)
{
    esp_wifi_stop();
    
    wifi_config_t wifi_ap_cfg = 
    {
        .ap = 
        {
            .ssid = "DEADBEEF_BitBoxV1_CFG",
            .ssid_len = (uint8_t)strlen("DEADBEEF_BitBoxV1_CFG"),
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}

void wifi_start_captive_portal(void)
{
    if(wifi_state_lock)
    {
        xSemaphoreTake(wifi_state_lock, portMAX_DELAY);
    }

    ESP_LOGI(TAG, "Iniciando captive portal...");

    portal_running = true;
    retry_num = 0;

    mqtt_deinit_app();

    wifi_dns_stop();
    if (http_server) 
    {
        httpd_stop(http_server);
        http_server = NULL;
    }

    wifi_init_ap(); 
    wifi_dns_start();
    http_server_start();

    start_portal_timeout();
    ESP_LOGI(TAG, "Captive portal ativo em http://192.168.4.1");

    if(wifi_state_lock)
    {
        xSemaphoreGive(wifi_state_lock);
    }
}

bool wifi_conn_init(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    wifi_state_lock = xSemaphoreCreateMutex();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    sta_netif = esp_netif_create_default_wifi_sta();
    wifi_ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg_init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg_init));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    config_button_init();

    xTaskCreate(config_task, "config_task", 4096, NULL, 10, &config_task_handle);

    ESP_LOGI(TAG, "Lendo configurações de rede...");

    sys_config_netw_t netw_cfg = { 0 };                        
    
    // Tenta carregar config
    if(app_config_netw_load(&netw_cfg) != ESP_OK) 
    {
        ESP_LOGW(TAG, "Nenhuma config encontrada. Abrindo Portal.");
        wifi_start_captive_portal();
        return false;
    }

    // --- LÓGICA OFFLINE ---
if (netw_cfg.offline_mode)
    {
        ESP_LOGW(TAG, ">>> MODO OFFLINE ATIVADO <<<");

        esp_wifi_stop();

#if CONFIG_PM_ENABLE
            esp_pm_config_t pm_config = 
            {
                .max_freq_mhz = 80,  
                .min_freq_mhz = 80,  
                .light_sleep_enable = true 
            };
            
            esp_err_t err = esp_pm_configure(&pm_config);
            
            if (err == ESP_OK) 
            {
                ESP_LOGI(TAG, "CPU Clock travado em 80MHz com Auto Light Sleep.");
            } 
            
            else 
            {
                ESP_LOGE(TAG, "Falha ao configurar PM: %s", esp_err_to_name(err));
            }
#endif

        return false;
    }

    // ----------------------

    ESP_LOGI(TAG, "Conectando em %s...", netw_cfg.ssid);
    wifi_config_t wifi_cfg = { 0 };
    memcpy(wifi_cfg.sta.ssid, netw_cfg.ssid, strlen(netw_cfg.ssid));
    memcpy(wifi_cfg.sta.password, netw_cfg.pass, strlen(netw_cfg.pass));

    return wifi_conn_init_sta(&wifi_cfg);
}
