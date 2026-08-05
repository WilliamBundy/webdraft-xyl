#pragma once
#include <stdint.h>

/* 
dict32.h, v1.0
a uint32->uint32 dictionary
largely intended for storing hashes to indices

dict32ActionHash variants work on hashes directly
dict32Action variants work on strings and will hash them every time

Every function is marked static inline, so no implemented macro... for now
*/

#define DICT32_OK 0

#ifdef DICT32_USE_SDL_LIBC
#define dict32realloc(ptr, newSize) SDL_realloc((ptr), (newSize))
#define dict32tempalloc(ptr, newSize) SDL_calloc(1, (newSize))
#define dict32calloc(n, size) SDL_calloc(n, size)
#define dict32free(ptr) SDL_free((ptr))
#define dict32tempfree(ptr) SDL_free((ptr))
#define dict32memset(ptr, v, sz) SDL_memset((ptr), (v), (sz))
#define dict32memcpy(dst, src, sz) SDL_memcpy((dst), (src), (sz))
#define dict32strlen(str) SDL_strlen(str)
#endif

#ifndef dict32realloc
#include <stdlib.h>
#include <string.h>
#define dict32realloc(ptr, newSize) realloc((ptr), (newSize))
#define dict32tempalloc(ptr, newSize) calloc(1, (newSize))
#define dict32calloc(n, size) calloc(n, size)
#define dict32free(ptr) free((ptr))
#define dict32tempfree(ptr) free((ptr))
#define dict32memset(ptr, v, sz) memset((ptr), (v), (sz))
#define dict32memcpy(dst, src, sz) memcpy((dst), (src), (sz))
#define dict32strlen(str) strlen(str)
#endif

static inline
uint32_t dict32mur3(const void *key, int len, uint32_t h) 
{
	//thanks, demetri!
	// main body, work on 32-bit blocks at a time
	for (int i=0;i<len/4;i++) 
	{
		uint32_t k = ((uint32_t*) key)[i]*0xcc9e2d51;
		k = ((k << 15) | (k >> 17))*0x1b873593;
		h = (((h^k) << 13) | ((h^k) >> 19))*5 + 0xe6546b64;
	}

	// load/mix up to 3 remaining tail bytes into a tail block
	uint32_t t = 0;
	uint8_t *tail = ((uint8_t*) key) + 4*(len/4); 
	switch(len & 3) 
	{
		case 3: t ^= tail[2] << 16;
		case 2: t ^= tail[1] <<  8;
		case 1: { 
			t ^= tail[0] <<  0;
			h ^= ((0xcc9e2d51*t << 15) | (0xcc9e2d51*t >> 17))*0x1b873593;
		}
	}

	// finalization mix, including key length
	h = ((h^len) ^ ((h^len) >> 16))*0x85ebca6b;
	h = (h ^ (h >> 13))*0xc2b2ae35;
	return h ^ (h >> 16); 
}

static inline
uint32_t dict32mur3_uint(uint32_t key, uint32_t h) 
{
	uint32_t k = key*0xcc9e2d51;
	k = ((k << 15) | (k >> 17))*0x1b873593;
	h = (((h^k) << 13) | ((h^k) >> 19))*5 + 0xe6546b64;

	h = ((h^4) ^ ((h^4) >> 16))*0x85ebca6b;
	h = (h ^ (h >> 13))*0xc2b2ae35;
	return h ^ (h >> 16); 
}

static inline
uint32_t dict32mur3_uint64(uint64_t key64, uint32_t h) 
{
	uint32_t key = key64 & 0xFFFFFFFFu;
	{
		uint32_t k = key*0xcc9e2d51;
		k = ((k << 15) | (k >> 17))*0x1b873593;
		h = (((h^k) << 13) | ((h^k) >> 19))*5 + 0xe6546b64;
	}

	key = key64 >> 32;
	{
		uint32_t k = key*0xcc9e2d51;
		k = ((k << 15) | (k >> 17))*0x1b873593;
		h = (((h^k) << 13) | ((h^k) >> 19))*5 + 0xe6546b64;
	}

	h = ((h^4) ^ ((h^4) >> 16))*0x85ebca6b;
	h = (h ^ (h >> 13))*0xc2b2ae35;
	return h ^ (h >> 16); 
}



enum
{
	DICT32_INVALID_ARG = -1,
	DICT32_NOT_FOUND = -2,
	DICT32_ALREADY_PRESENT = -3,
	DICT32_OUT_OF_SPACE = -4,
};

#define DICT32_ADD (1<<8)
#define DICT32_UPDATE (1<<9)
#define DICT32_SET (DICT32_ADD|DICT32_UPDATE)

#define DICT32_PRIME     0x9E3779B1u
#define DICT32_PRIME_F1  0x85EBCA77u
#define DICT32_PRIME_F2  0xC2B2AE3Du

// if slotsFilled > ((numSlots * N) / D): resize
#define DICT32_RESIZE_FACTOR_N 3
#define DICT32_RESIZE_FACTOR_D 4

#define DICT_NO_RESIZE 1<<0

typedef struct dict32
{
	uint32_t slotMask;
	uint32_t slotsFilled;
	uint32_t* keys;
	uint32_t* vals;

	uint32_t flags;
} dict32;

static inline
dict32* dict32Resize(dict32* dict, int numSlots)
{
	if(dict->flags & DICT_NO_RESIZE) return dict;
	uint32_t oldNumSlots = dict->slotMask + 1;
	uint32_t* oldKeys = nullptr;
	uint32_t* oldVals = nullptr;
	if(dict) {
		oldKeys = dict32tempalloc(nullptr, oldNumSlots * sizeof(uint32_t));
		oldVals = dict32tempalloc(nullptr, oldNumSlots * sizeof(uint32_t));

		dict32memcpy(oldKeys, dict->keys, oldNumSlots * sizeof(uint32_t));
		dict32memcpy(oldVals, dict->vals, oldNumSlots * sizeof(uint32_t));
	}

	size_t dictSize = sizeof(dict32) + sizeof(uint32_t) * 2 * numSlots;
	dict32 backup = *dict;
	dict = dict32realloc(dict, dictSize);
	if(dict == nullptr) {
		return nullptr;
	}
	dict32memset(dict, 0, dictSize);
	*dict = backup;
	dict->keys = (uint32_t*)(dict + 1);
	dict->vals = dict->keys + numSlots;
	dict->slotMask = numSlots - 1;

	if(oldKeys) {
		uint32_t mask = dict->slotMask;
		for(uint32_t i = 0; i < oldNumSlots; ++i) {
			uint32_t hash = oldKeys[i];
			if(hash == -1 || hash == 0) continue;

			uint32_t slot = hash & mask;
			uint32_t originalSlot = slot;

			while(dict->keys[slot & mask] != hash && dict->keys[slot & mask] != 0) {
				slot++;
				if((slot & mask) == originalSlot) {
					break;
				}
			}

			if((slot & mask) == originalSlot && slot != originalSlot) {
				// TODO error handling
				return nullptr;
			}

			dict->keys[slot & mask] = hash;
			dict->vals[slot & mask] = oldVals[i];
			dict->slotsFilled++;
		}

		dict32tempfree(oldKeys);
		dict32tempfree(oldVals);
	}

	return dict;
}

static inline
dict32* dict32Create(uint32_t reservedSize)
{
	int numSlots = 1 << (32 - __builtin_clz(reservedSize));
	size_t dictSize = sizeof(dict32) + sizeof(uint32_t) * 2 * numSlots;
	dict32* dict = dict32calloc(1, dictSize);
	dict->keys = calloc(numSlots, sizeof(uint32_t));//(uint32_t*)(dict + 1);
	dict->vals = calloc(numSlots, sizeof(uint32_t));//dict->keys + numSlots;
	dict->slotsFilled = 0;
	dict->slotMask = numSlots - 1;
	return dict;
}

static inline
size_t dict32GetSize(uint32_t reservedSize)
{
	int numSlots = 1 << (32 - __builtin_clz(reservedSize));
	return sizeof(dict32) + sizeof(uint32_t) * 2 * numSlots;
}

static inline
dict32* dict32Init(void* mem, uint32_t reservedSize)
{
	int numSlots = 1 << (32 - __builtin_clz(reservedSize));
	dict32* dict = mem;
	dict->keys = (uint32_t*)(dict + 1);
	dict->vals = (uint32_t*)(dict->keys + numSlots);
	dict->slotsFilled = 0;
	dict->slotMask = numSlots - 1;
	dict->flags |= DICT_NO_RESIZE;
	return dict;
}

static inline 
bool dict32SlotIsValid(dict32* dict, int slot)
{
	uint32_t key = dict->keys[slot];
	return !(key == 0 || key == -1);
}

static inline
void dict32Clear(dict32* dict)
{
	dict32memset(dict->keys, 0, sizeof(uint32_t) * (dict->slotMask + 1));
	dict32memset(dict->vals, 0, sizeof(uint32_t) * (dict->slotMask + 1));
	dict->slotsFilled = 0;
}

static inline
int dict32FindHash(dict32* dict, uint32_t hash, bool skipDeleted, uint32_t* slotOut)
{
	uint32_t mask = dict->slotMask;
	uint32_t slot = hash & mask;
	uint32_t originalSlot = slot;
	uint32_t lastdeleted = -1;

	while(dict->keys[slot & mask] != hash && dict->keys[slot & mask] != 0) {
		if(dict->keys[slot & mask] == -1) {
			lastdeleted = slot;
		}
		slot++;
		if((slot & mask) == originalSlot) {
			break;
		}
	}

	if((slot & mask) == originalSlot && slot != originalSlot) {
		if(!skipDeleted && lastdeleted != -1) {
			slot = lastdeleted;
		} else {
			return DICT32_NOT_FOUND;
		}
	}

	//if(dict->keys[slot & mask] == -1) {
	//	return DICT32_NOT_FOUND;
	//}

	*slotOut = slot & mask;

	return DICT32_OK;
}

static inline
int dict32SetHash(dict32** pdict, uint32_t hash, uint32_t val, int mode)
{
	dict32* dict = *pdict;
	uint32_t slot;
	int ret = dict32FindHash(dict, hash, false, &slot);

	if(ret != DICT32_NOT_FOUND && slot > dict->slotMask) {
		SDL_Log("ERROR: find hash returned invalid slot %u/%u", slot, dict->slotMask);
		//TODO error reporting
	}

	if(ret == DICT32_NOT_FOUND) {
		dict = dict32Resize(dict, (dict->slotMask + 1) * 2);
		*pdict = dict;
		ret = dict32FindHash(dict, hash, false, &slot);
		if(ret == DICT32_NOT_FOUND) {
			//SDL_Log("ERROR: set failed after resize");
			//TODO error reporting
			return DICT32_OUT_OF_SPACE;
		}
	}



	bool adding = false;
	if((mode & DICT32_ADD) && dict->keys[slot] != hash) {
		dict->slotsFilled++;
		adding = true;
	}

	if((!adding && (mode & DICT32_UPDATE)) || (mode & DICT32_ADD)) {
		dict->keys[slot] = hash;
		dict->vals[slot] = val;
	}

	if(dict->slotsFilled > (dict->slotMask + 1) * DICT32_RESIZE_FACTOR_N / DICT32_RESIZE_FACTOR_D) {
		dict = dict32Resize(dict, (dict->slotMask + 1) * 2);
		*pdict = dict;
	}

	return DICT32_OK;
}

static inline
int dict32GetHash(dict32* dict, uint32_t hash, uint32_t* val)
{
	uint32_t slot;
	int ret = dict32FindHash(dict, hash, true, &slot);


	if(ret == DICT32_NOT_FOUND) {
		return ret;
	}

	if(dict->keys[slot] != hash) {
		return DICT32_NOT_FOUND;
	}

	if(val) *val = dict->vals[slot];
	return DICT32_OK;
}

static inline
int dict32DeleteHash(dict32* dict, uint32_t hash)
{
	uint32_t slot;
	int ret = dict32FindHash(dict, hash, true, &slot);


	if(ret == DICT32_NOT_FOUND || dict->keys[slot] == 0) {
		return DICT32_NOT_FOUND;
	}

	dict->keys[slot] = -1;
	dict->vals[slot] = 0;
	dict->slotsFilled--;

	return DICT32_OK;
}

static inline
int dict32Set(dict32** pdict, const char* key, uint32_t val)
{
	return dict32SetHash(pdict, dict32mur3(key, dict32strlen(key), DICT32_PRIME), val, DICT32_SET);
}

static inline
int dict32Add(dict32** pdict, const char* key, uint32_t val)
{
	return dict32SetHash(pdict, dict32mur3(key, dict32strlen(key), DICT32_PRIME), val, DICT32_ADD);
}

static inline
int dict32Update(dict32** pdict, const char* key, uint32_t val)
{
	return dict32SetHash(pdict, dict32mur3(key, dict32strlen(key), DICT32_PRIME), val, DICT32_UPDATE);
}
static inline
int dict32Get(dict32* dict, const char* key, uint32_t* val)
{
	return dict32GetHash(dict, dict32mur3(key, dict32strlen(key), DICT32_PRIME), val);
}
static inline
int dict32Delete(dict32* dict, const char* key)
{
	return dict32DeleteHash(dict, dict32mur3(key, dict32strlen(key), DICT32_PRIME));
}

static inline
int dict32SetUint(dict32** pdict, uint32_t key, uint32_t val)
{
	return dict32SetHash(pdict, dict32mur3_uint(key, DICT32_PRIME), val, DICT32_SET);
}

static inline
int dict32AddUint(dict32** pdict, uint32_t key, uint32_t val)
{
	return dict32SetHash(pdict, dict32mur3_uint(key, DICT32_PRIME), val, DICT32_ADD);
}

static inline
int dict32UpdateUint(dict32** pdict, uint32_t key, uint32_t val)
{
	return dict32SetHash(pdict, dict32mur3_uint(key, DICT32_PRIME), val, DICT32_UPDATE);
}
static inline
int dict32GetUint(dict32* dict, uint32_t key, uint32_t* val)
{
	return dict32GetHash(dict, dict32mur3_uint(key, DICT32_PRIME), val);
}
static inline
int dict32DeleteUint(dict32* dict, uint32_t key)
{
	return dict32DeleteHash(dict, dict32mur3_uint(key, DICT32_PRIME));
}


static inline
int dict32SetUint64(dict32** pdict, uint64_t key, uint32_t val)
{
	return dict32SetHash(pdict, dict32mur3_uint64(key, DICT32_PRIME), val, DICT32_SET);
}

static inline
int dict32AddUint64(dict32** pdict, uint64_t key, uint32_t val)
{
	return dict32SetHash(pdict, dict32mur3_uint64(key, DICT32_PRIME), val, DICT32_ADD);
}

static inline
int dict32UpdateUint64(dict32** pdict, uint64_t key, uint32_t val)
{
	return dict32SetHash(pdict, dict32mur3_uint64(key, DICT32_PRIME), val, DICT32_UPDATE);
}
static inline
int dict32GetUint64(dict32* dict, uint64_t key, uint32_t* val)
{
	return dict32GetHash(dict, dict32mur3_uint64(key, DICT32_PRIME), val);
}
static inline
int dict32DeleteUint64(dict32* dict, uint64_t key)
{
	return dict32DeleteHash(dict, dict32mur3_uint64(key, DICT32_PRIME));
}


