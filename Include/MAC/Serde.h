#pragma once

#ifndef __SERDE_H__
#define __SERDE_H__

struct MACFrame;
struct MACHeader;
class Frame;

typedef unsigned char uint8_t;

/* Conversion from MAC Frame to Byte array (inplace) */

uint8_t* serialize(MACFrame* frame);

MACFrame* deserialize(uint8_t* data);

Frame convertFrame(MACFrame* macframe);

void convertMACFrame(const Frame& phy, MACFrame* macFrame);

MACHeader* headerView(uint8_t* data);

#endif