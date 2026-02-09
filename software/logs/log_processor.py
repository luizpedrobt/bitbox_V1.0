import struct
import os
import mmap
import glob
import argparse
import shutil
import sys

# ========= CONFIGURAÇÕES DO PROTOCOLO =========
HEADER_MAGIC = 0xDEADBEEF
SD_LOG_UART = 1
SD_LOG_GPIO = 2

# Structs
HDR_FMT = "<I Q B B"     # header, time_us, log_type, periph_num
HDR_SIZE = struct.calcsize(HDR_FMT)
UART_LEN_FMT = "<H"
UART_LEN_SIZE = struct.calcsize(UART_LEN_FMT)
GPIO_FMT = "<B B"
GPIO_SIZE = struct.calcsize(GPIO_FMT)

# Formatos de Decode (TXT)
GPIO_TXT_FMT = "<Q B B"
UART_TXT_TS_FMT = "<Q"

# ========= FUNÇÕES DE DECODE (TXT) =========

def fmt_payload_uart(buf):
    """Converte bytes para string ASCII amigável, escapando não-imprimíveis."""
    out = []
    for b in buf:
        if b == 0: out.append("\\0") # Representação visual do null
        elif 32 <= b <= 126: out.append(chr(b)) # Caracteres ASCII imprimíveis
        elif b == 10: out.append("\\n") # Newline
        elif b == 13: out.append("\\r") # Carriage Return
        else: out.append(f"\\x{b:02X}") # Hex para o resto
    return "".join(out)

def decode_to_txt(input_dir, output_dir):
    print(f"\n--- Iniciando decodificação e impressão ---")
    bin_files = glob.glob(os.path.join(input_dir, "*.bin"))
    bin_files.sort() # Ordena para ficar bonito no terminal

    if not bin_files:
        print("Nenhum arquivo binário intermediário encontrado para decodificar.")
        return

    for bin_file in bin_files:
        filename = os.path.basename(bin_file)
        name_no_ext = os.path.splitext(filename)[0]
        txt_path = os.path.join(output_dir, name_no_ext + ".txt")

        # Cabeçalho visual no terminal
        print(f"\n{'='*60}")
        print(f"ARQUIVO: {filename} -> {os.path.basename(txt_path)}")
        print(f"{'='*60}")

        if filename.startswith("gpio"):
            # Lógica GPIO TXT
            with open(bin_file, "rb") as f_in, open(txt_path, "w") as f_out:
                header_line = "Timestamp (us) | Edge | Level"
                sep_line = "-" * 40
                
                # Escreve header no arquivo e tela
                f_out.write(header_line + "\n" + sep_line + "\n")
                print(f"{header_line}\n{sep_line}")

                idx = 0
                while chunk := f_in.read(struct.calcsize(GPIO_TXT_FMT)):
                    ts, edge, level = struct.unpack(GPIO_TXT_FMT, chunk)
                    
                    # Formata a linha
                    line_content = f"[{idx:05d}] {ts:<12} | edge={edge} level={level}"
                    
                    # Salva e Imprime
                    f_out.write(line_content + "\n")
                    print(line_content)
                    
                    idx += 1

        elif filename.startswith("uart"):
            # Lógica UART TXT
            with open(bin_file, "rb") as f_in, open(txt_path, "w") as f_out:
                header_line = "Timestamp (us) | Payload (ASCII)"
                sep_line = "-" * 60
                
                # Escreve header no arquivo e tela
                f_out.write(header_line + "\n" + sep_line + "\n")
                print(f"{header_line}\n{sep_line}")

                idx = 0
                ts_size = struct.calcsize(UART_TXT_TS_FMT)
                while True:
                    ts_data = f_in.read(ts_size)
                    if not ts_data: break
                    ts = struct.unpack(UART_TXT_TS_FMT, ts_data)[0]
                    
                    payload = bytearray()
                    while (byte := f_in.read(1)):
                        payload.append(byte[0])
                        if byte[0] == 0: break
                    
                    # Formata a linha
                    decoded_str = fmt_payload_uart(payload)
                    line_content = f"[{idx:05d}] {ts:<12} | {decoded_str}"
                    
                    # Salva e Imprime
                    f_out.write(line_content + "\n")
                    print(line_content)
                    
                    idx += 1
    print("\n--- Processamento concluído ---")

# ========= PROCESSADOR PRINCIPAL (SPLITTER) =========

def process_log(input_file, output_dir, keep_bins=True):
    # Garante que o diretório de saída existe
    if not os.path.exists(output_dir):
        print(f"Criando diretório de saída: {output_dir}")
        os.makedirs(output_dir)

    print(f"Lendo arquivo bruto: {input_file}")
    
    out_files = {}

    def get_file_handle(prefix, num):
        key = f"{prefix}{num}"
        if key not in out_files:
            path = os.path.join(output_dir, f"{key}.bin")
            out_files[key] = open(path, "wb") 
        return out_files[key]

    try:
        f = open(input_file, "rb")
    except FileNotFoundError:
        print(f"Erro: Arquivo '{input_file}' não encontrado.")
        return

    if os.path.getsize(input_file) == 0:
        print("Arquivo vazio.")
        f.close()
        return

    with f:
        with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as mm:
            offset = 0
            total_size = len(mm)
            
            while offset + HDR_SIZE <= total_size:
                check_magic = struct.unpack_from("<I", mm, offset)[0]
                
                if check_magic != HEADER_MAGIC:
                    new_offset = mm.find(struct.pack("<I", HEADER_MAGIC), offset + 1)
                    if new_offset == -1:
                        break 
                    offset = new_offset
                    continue

                try:
                    _, time_us, log_type, periph = struct.unpack_from(HDR_FMT, mm, offset)
                except:
                    break
                
                p = offset + HDR_SIZE

                # --- UART ---
                if log_type == SD_LOG_UART:
                    if p + UART_LEN_SIZE > total_size: break
                    payload_len = struct.unpack_from(UART_LEN_FMT, mm, p)[0]
                    p += UART_LEN_SIZE
                    
                    if payload_len > 2048 or p + payload_len > total_size:
                        offset += 1 
                        continue
                        
                    payload = mm[p : p + payload_len]
                    
                    f_out = get_file_handle("uart", periph)
                    f_out.write(struct.pack("<Q", time_us))
                    f_out.write(payload)
                    f_out.write(b"\x00") 
                    
                    offset = p + payload_len

                # --- GPIO ---
                elif log_type == SD_LOG_GPIO:
                    if p + GPIO_SIZE > total_size: break
                    edge, level = struct.unpack_from(GPIO_FMT, mm, p)
                    
                    f_out = get_file_handle("gpio", periph)
                    f_out.write(struct.pack("<Q B B", time_us, edge, level))
                    
                    offset = p + GPIO_SIZE
                
                else:
                    offset += 1

    # Fecha arquivos binários
    for f_h in out_files.values():
        f_h.close()
    
    print(f"Separação binária concluída. Arquivos gerados em '{output_dir}/'")
    
    # Chama o decoder (que agora imprime na tela)
    # Passamos o output_dir tanto como fonte dos bins quanto destino dos txts
    decode_to_txt(output_dir, output_dir)

    if not keep_bins:
        print("Removendo binários temporários...")
        for k in out_files.keys():
            try: os.remove(os.path.join(output_dir, k + ".bin"))
            except: pass

# ========= ENTRY POINT =========

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="BitBox Log Parser & Viewer")
    
    parser.add_argument("logfile", help="Caminho do arquivo .BIN bruto (ex: LOG12.BIN)")
    
    # Opção para definir diretório de saída
    parser.add_argument("--out", default="logs_processados", 
                        help="Diretório onde os arquivos TXT serão salvos (Padrão: logs_processados)")
    
    parser.add_argument("--clean", action="store_true", 
                        help="Apagar arquivos .bin intermediários após gerar os .txt")
    
    args = parser.parse_args()
    
    process_log(args.logfile, args.out, keep_bins=not args.clean)