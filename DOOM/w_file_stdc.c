//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	WAD I/O functions.
//

#include <stdio.h>
#include <string.h>
#include "ff.h"         // Наша библиотека FatFS!
#include "m_misc.h"
#include "w_file.h"
#include "z_zone.h"

typedef struct
{
    wad_file_t wad;
    FIL fstream;        // Наша структура файла FatFS вместо старого FILE*!
} stdc_wad_file_t;

extern wad_file_class_t stdc_wad_file;

static wad_file_t *W_StdC_OpenFile(char *path)
{
    stdc_wad_file_t *result;
    FRESULT res;

    // 1. Отрезаем любые префиксы папок ПК (например "./" или "c:\"), оставляем чистое имя
    char *filename = strrchr(path, '/');
    if (!filename) filename = strrchr(path, '\\');
    if (filename) path = filename + 1;
    if (path[0] == '.' && path[1] == '/') path += 2;

    // 2. Выделяем память под структуру файла в нашей памяти
    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
    if (result == NULL) return NULL;

    // 3. Открываем файл через FatFS на нашей MicroSD карте!
    res = f_open(&result->fstream, path, FA_READ);
    if (res != FR_OK)
    {
        Z_Free(result);
        return NULL; // Файл не найден
    }

    // 4. Заполняем параметры файла
    result->wad.file_class = &stdc_wad_file;
    result->wad.mapped = NULL;
    result->wad.length = f_size(&result->fstream); // Размер файла через макрос FatFS!

    return &result->wad;
}

static void W_StdC_CloseFile(wad_file_t *wad)
{
    stdc_wad_file_t *stdc_wad = (stdc_wad_file_t *) wad;

    // Закрываем файл через FatFS
    f_close(&stdc_wad->fstream);
    Z_Free(stdc_wad);
}

// Чтение куска данных со смещением (offset)
// Создаем во внутренней памяти выровненный буфер-посредник на 1 сектор флешки!
#if defined(__GNUC__)
__attribute__((aligned(32))) static uint8_t sd_proxy_buffer[512];
#endif

size_t W_StdC_Read(wad_file_t *wad, unsigned int offset,
                   void *buffer, size_t buffer_len)
{
    stdc_wad_file_t *stdc_wad = (stdc_wad_file_t *) wad;
    UINT bytes_read = 0;
    UINT total_read = 0;
    BYTE *dest_ptr = (BYTE *)buffer;

    f_lseek(&stdc_wad->fstream, offset);

    // Читаем большими кусками через наш выровненный прокси-буфер!
    while (buffer_len > 0)
    {
        // Читаем максимум 512 байт за раз
        size_t chunk_size = (buffer_len > 512) ? 512 : buffer_len;

        // Флешка безопасно пишет во внутренний выровненный буфер:
        FRESULT res = f_read(&stdc_wad->fstream, sd_proxy_buffer, chunk_size, &bytes_read);

        if (res != FR_OK || bytes_read == 0) {
            break; // Конец файла или ошибка
        }

        // Процессор мгновенно копирует данные из буфера в PSRAM!
        memcpy(dest_ptr, sd_proxy_buffer, bytes_read);

        dest_ptr += bytes_read;
        total_read += bytes_read;
        buffer_len -= bytes_read;
    }

    return (size_t)total_read;
}
wad_file_class_t stdc_wad_file = 
{
    W_StdC_OpenFile,
    W_StdC_CloseFile,
    W_StdC_Read,
};
