#ifndef BITBOARD_H
#define BITBOARD_H

#include <cstdint>
#include <intrin.h> 

class Bitboard {

public:
     uint64_t bitboard; 

     Bitboard() : bitboard(0){}

     __forceinline void setSquare(int square){
        bitboard |= (1ULL << square);
     }

     __forceinline void clearSquare(int square){
        bitboard &= ~(1ULL << square);
     }

     __forceinline bool isSet(int square) const {
         return bitboard & (1ULL << square);
     }

     __forceinline  int count() const {
         return _mm_popcnt_u64(bitboard); 
     }
};

#endif
