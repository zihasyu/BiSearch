/*
 * @Author: Helix0503 834991203@qq.com
 * @Date: 2024-01-08 16:41:48
 * @LastEditors: Helix && 834991203@qq.com
 * @LastEditTime: 2024-01-31 21:04:39
 * @FilePath: /LocalDedupSim/include/struct.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef MY_STRUCT_H
#define MY_STRUCT_H

#include "define.h"

typedef struct
{
    uint64_t chunkID;
    uint32_t chunkSize;
    uint32_t saveSize;
    uint8_t chunkContent[MAX_CHUNK_SIZE];
} Chunk_t;

typedef struct
{
    uint64_t size;
    uint64_t chunkNum;
    uint64_t containerId;
    uint8_t data[CONTAINER_MAX_SIZE];
} Container_t;

typedef struct
{
    uint64_t chunkId;
    size_t chunkType;
    uint64_t offset;
} Recipe_t;
#endif