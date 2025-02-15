#include "Bitboard.h"
#include <intrin.h> 

void Bitboard::setSquare(int square){
    bitboard |= (1ULL << square);
}
void Bitboard::clearSquare(int square){
    bitboard &= ~(1ULL << square);
}

bool Bitboard::isSet(int square) const{
    return bitboard & (1ULL << square);
}

int Bitboard::count() const {
    return _mm_popcnt_u64(bitboard); 
}