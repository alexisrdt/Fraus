#ifndef FRAUS_ARENA_H
#define FRAUS_ARENA_H

#include <stddef.h>
#include <stdint.h>

static const size_t frSizeMax = PTRDIFF_MAX < SIZE_MAX ? PTRDIFF_MAX : SIZE_MAX;

typedef struct FrRegion
{
	struct FrRegion* previous;
	struct FrRegion* next;

	size_t capacity;
	size_t size;

	alignas(max_align_t) unsigned char data[];
} FrRegion;

static const size_t frArenaSizeMax = frSizeMax - sizeof(FrRegion);

typedef struct FrArena
{
	FrRegion* start;
	FrRegion* end;
} FrArena;

typedef struct FrArenaSave
{
	FrRegion* previous;
	size_t size;
} FrArenaSave;

void frArenaCreate(FrArena* arena);
void frArenaFree(FrArena* arena);

void* frArenaAllocate(FrArena* arena, size_t size, size_t alignment);
void frArenaClear(FrArena* arena);

FrArenaSave frArenaSave(const FrArena* arena);
void frArenaRestore(FrArena* arena, FrArenaSave save);

#endif
