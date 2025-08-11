#ifndef BITBOARD_H
#define BITBOARD_H

#include <cstdint>

class Bitboard {

public:
     uint64_t bitboard; 

     Bitboard() : bitboard(0){}
     void setSquare(int square);
     void clearSquare(int square);
     __forceinline bool isSet(int square) const {
         return bitboard & (1ULL << square);
     }
     int count() const ;
};

#endif
