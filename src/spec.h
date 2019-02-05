#ifndef __SPEC_H__
#define __SPEC_H__
#include <stdlib.h>
#include <string.h>

#include "default_gc.h"
#include "redismodule.h"
#include "doc_table.h"
#include "trie/trie_type.h"
#include "sortable.h"
#include "stopwords.h"
#include "gc.h"
#include "synonym_map.h"
#include "query_error.h"
#include "field_spec.h"
#include "util/dict.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUMERIC_STR "NUMERIC"
#define GEO_STR "GEO"

#define SPEC_NOOFFSETS_STR "NOOFFSETS"
#define SPEC_NOFIELDS_STR "NOFIELDS"
#define SPEC_NOFREQS_STR "NOFREQS"
#define SPEC_NOHL_STR "NOHL"
#define SPEC_SCHEMA_STR "SCHEMA"
#define SPEC_SCHEMA_EXPANDABLE_STR "MAXTEXTFIELDS"
#define SPEC_TEMPORARY_STR "TEMPORARY"
#define SPEC_TEXT_STR "TEXT"
#define SPEC_WEIGHT_STR "WEIGHT"
#define SPEC_NOSTEM_STR "NOSTEM"
#define SPEC_PHONETIC_STR "PHONETIC"
#define SPEC_TAG_STR "TAG"
#define SPEC_SORTABLE_STR "SORTABLE"
#define SPEC_STOPWORDS_STR "STOPWORDS"
#define SPEC_NOINDEX_STR "NOINDEX"
#define SPEC_SEPARATOR_STR "SEPARATOR"

static const char *SpecTypeNames[] = {[FIELD_FULLTEXT] = SPEC_TEXT_STR,
                                      [FIELD_NUMERIC] = NUMERIC_STR,
                                      [FIELD_GEO] = GEO_STR,
                                      [FIELD_TAG] = SPEC_TAG_STR};

#define INDEX_SPEC_KEY_PREFIX "idx:"
#define INDEX_SPEC_KEY_FMT INDEX_SPEC_KEY_PREFIX "%s"

#define SPEC_MAX_FIELDS 1024
#define SPEC_MAX_FIELD_ID (sizeof(t_fieldMask) * 8)
// The threshold after which we move to a special encoding for wide fields
#define SPEC_WIDEFIELD_THRESHOLD 32

typedef struct {
  size_t numDocuments;
  size_t numTerms;
  size_t numRecords;
  size_t invertedSize;
  size_t invertedCap;
  size_t skipIndexesSize;
  size_t scoreIndexesSize;
  size_t offsetVecsSize;
  size_t offsetVecRecords;
  size_t termsSize;
} IndexStats;

typedef enum {
  Index_StoreTermOffsets = 0x01,
  Index_StoreFieldFlags = 0x02,

  // Was StoreScoreIndexes, but these are always stored, so this option is unused
  Index__Reserved1 = 0x04,
  Index_HasCustomStopwords = 0x08,
  Index_StoreFreqs = 0x010,
  Index_StoreNumeric = 0x020,
  Index_StoreByteOffsets = 0x40,
  Index_WideSchema = 0x080,
  Index_HasSmap = 0x100,
  Index_Temporary = 0x200,
  Index_DocIdsOnly = 0x00,

  // If any of the fields has phonetics. This is just a cache for quick lookup
  Index_HasPhonetic = 0x400
} IndexFlags;

/**
 * This "ID" type is independent of the field mask, and is used to distinguish
 * between one field and another field. For now, the ID is the position in
 * the array of fields - a detail we'll try to hide.
 */
typedef uint16_t FieldSpecDedupeArray[SPEC_MAX_FIELDS];

#define INDEX_DEFAULT_FLAGS \
  Index_StoreFreqs | Index_StoreTermOffsets | Index_StoreFieldFlags | Index_StoreByteOffsets

#define INDEX_STORAGE_MASK                                                                  \
  (Index_StoreFreqs | Index_StoreFieldFlags | Index_StoreTermOffsets | Index_StoreNumeric | \
   Index_WideSchema)

#define INDEX_CURRENT_VERSION 13
// Those versions contains doc table as array, we modified it to be array of linked lists
#define INDEX_MIN_COMPACTED_DOCTABLE_VERSION 12
#define INDEX_MIN_COMPAT_VERSION 2
// Versions below this always store the frequency
#define INDEX_MIN_NOFREQ_VERSION 6
// Versions below this encode field ids as the actual value,
// above - field ides are encoded as their exponent (bit offset)
#define INDEX_MIN_WIDESCHEMA_VERSION 7

// Versions below this didn't know tag indexes
#define INDEX_MIN_TAGFIELD_VERSION 8

// Versions below this one don't save the document len when serializing the table
#define INDEX_MIN_DOCLEN_VERSION 9

#define INDEX_MIN_BINKEYS_VERSION 10

// Versions below this one do not contains expire information
#define INDEX_MIN_EXPIRE_VERSION 13

#define Index_SupportsHighlight(spec) \
  (((spec)->flags & Index_StoreTermOffsets) && ((spec)->flags & Index_StoreByteOffsets))

#define FIELD_BIT(fs) (((t_fieldMask)1) << (fs)->textOpts.id)

#define VALUE_NOT_FOUND 0
#define STR_VALUE_TYPE 1
#define DOUBLE_VALUE_TYPE 2
typedef int (*GetValueCallback)(void *ctx, const char *fieldName, const void *id, char **strVal,
                                double *doubleVal);

typedef struct {
  char *name;
  FieldSpec *fields;
  int numFields;

  IndexStats stats;
  IndexFlags flags;

  Trie *terms;

  RSSortingTable *sortables;

  DocTable docs;

  StopWordList *stopwords;

  GCContext *gc;

  SynonymMap *smap;

  uint64_t uniqueId;

  RedisModuleCtx *strCtx;
  RedisModuleString **indexStrs;
  struct IndexSpecCache *spcache;
  long long timeout;
  dict *keysDict;
  long long minPrefix;
  long long maxPrefixExpansions;  // -1 unlimited
  GetValueCallback getValue;
  void *getValueCtx;
  size_t textFields;
} IndexSpec;

extern RedisModuleType *IndexSpecType;

/**
 * This lightweight object contains a COPY of the actual index spec.
 * This makes it safe for other modules to use for information such as
 * field names, WITHOUT worrying about the index schema changing.
 *
 * If the index schema changes, this object is simply recreated rather
 * than modified, making it immutable.
 *
 * It is freed when its reference count hits 0
 */
typedef struct IndexSpecCache {
  FieldSpec *fields;
  size_t nfields;
  size_t refcount;
} IndexSpecCache;

/**
 * Retrieves the current spec cache from the index, incrementing its
 * reference count by 1. Use IndexSpecCache_Decref to free
 */
IndexSpecCache *IndexSpec_GetSpecCache(const IndexSpec *spec);

/**
 * Decrement the reference count of the spec cache. Should be matched
 * with a previous call of GetSpecCache()
 */
void IndexSpecCache_Decref(IndexSpecCache *cache);

/**
 * Create a new copy of the spec cache from the current index spec
 */
IndexSpecCache *IndexSpec_BuildSpecCache(const IndexSpec *spec);

/*
 * Get a field spec by field name. Case insensitive!
 * Return the field spec if found, NULL if not
 */
const FieldSpec *IndexSpec_GetField(const IndexSpec *spec, const char *name, size_t len);

/**
 * Case-sensitive version of GetField()
 */
const FieldSpec *IndexSpec_GetFieldCase(const IndexSpec *spec, const char *name, size_t n);

const char *GetFieldNameByBit(const IndexSpec *sp, t_fieldMask id);

/* Get the field bitmask id of a text field by name. Return 0 if the field is not found or is not a
 * text field */
t_fieldMask IndexSpec_GetFieldBit(IndexSpec *spec, const char *name, size_t len);

/**
 * Check if phonetic matching is enabled on any field within the fieldmask.
 * Returns true if any field has phonetics, and false if none of the fields
 * require it.
 */
int IndexSpec_CheckPhoneticEnabled(const IndexSpec *sp, t_fieldMask fm);

/* Get a sortable field's sort table index by its name. return -1 if the field was not found or is
 * not sortable */
int IndexSpec_GetFieldSortingIndex(IndexSpec *sp, const char *name, size_t len);

/* Initialize some index stats that might be useful for scoring functions */
void IndexSpec_GetStats(IndexSpec *sp, RSIndexStats *stats);
/*
 * Parse an index spec from redis command arguments.
 * Returns REDISMODULE_ERR if there's a parsing error.
 * The command only receives the relvant part of argv.
 *
 * The format currently is <field> <weight>, <field> <weight> ...
 */
IndexSpec *IndexSpec_ParseRedisArgs(RedisModuleCtx *ctx, RedisModuleString *name,
                                    RedisModuleString **argv, int argc, QueryError *status);

FieldSpec **getFieldsByType(IndexSpec *spec, FieldType type);
int isRdbLoading(RedisModuleCtx *ctx);

/* Create a new index spec from redis arguments, set it in a redis key and start its GC.
 * If an error occurred - we set an error string in err and return NULL.
 */
IndexSpec *IndexSpec_CreateNew(RedisModuleCtx *ctx, RedisModuleString **argv, int argc,
                               QueryError *status);

/* Start the garbage collection loop on the index spec */
void IndexSpec_StartGC(RedisModuleCtx *ctx, IndexSpec *sp, float initialHZ);

/* Same as above but with ordinary strings, to allow unit testing */
IndexSpec *IndexSpec_Parse(const char *name, const char **argv, int argc, QueryError *status);
FieldSpec *IndexSpec_CreateField(IndexSpec *sp);

/* Add fields to a redis schema */
int IndexSpec_AddFields(IndexSpec *sp, const char **argv, int argc, QueryError *status);
int IndexSpec_AddFieldsRedisArgs(IndexSpec *sp, RedisModuleString **argv, int argc,
                                 QueryError *status);

IndexSpec *IndexSpec_Load(RedisModuleCtx *ctx, const char *name, int openWrite);

IndexSpec *IndexSpec_LoadEx(RedisModuleCtx *ctx, RedisModuleString *formattedKey, int openWrite,
                            RedisModuleKey **keyp);

// Global hook called when an index spec is created
extern void (*IndexSpec_OnCreate)(const IndexSpec *sp);

int IndexSpec_AddTerm(IndexSpec *sp, const char *term, size_t len);

/* Get a random term from the index spec using weighted random. Weighted random is done by sampling
 * N terms from the index and then doing weighted random on them. A sample size of 10-20 should be
 * enough */
char *IndexSpec_GetRandomTerm(IndexSpec *sp, size_t sampleSize);
/*
 * Free an indexSpec. This doesn't free the spec itself as it's not allocated by the parser
 * and should be on the request's stack
 */
void IndexSpec_Free(void *spec);

/** Delete the redis key from Redis */
void IndexSpec_FreeWithKey(IndexSpec *spec, RedisModuleCtx *ctx);

/* Parse a new stopword list and set it. If the parsing fails we revert to the default stopword
 * list, and return 0 */
int IndexSpec_ParseStopWords(IndexSpec *sp, RedisModuleString **strs, size_t len);

/* Return 1 if a term is a stopword for the specific index */
int IndexSpec_IsStopWord(IndexSpec *sp, const char *term, size_t len);

/** Returns a string suitable for indexes. This saves on string creation/destruction */
RedisModuleString *IndexSpec_GetFormattedKey(IndexSpec *sp, const FieldSpec *fs);
RedisModuleString *IndexSpec_GetFormattedKeyByName(IndexSpec *sp, const char *s);

IndexSpec *NewIndexSpec(const char *name);
int IndexSpec_AddField(IndexSpec *sp, FieldSpec *fs);
void *IndexSpec_RdbLoad(RedisModuleIO *rdb, int encver);
void IndexSpec_RdbSave(RedisModuleIO *rdb, void *value);
void IndexSpec_Digest(RedisModuleDigest *digest, void *value);
int IndexSpec_RegisterType(RedisModuleCtx *ctx);
// void IndexSpec_Free(void *value);

/*
 * Parse the field mask passed to a query, map field names to a bit mask passed down to the
 * execution engine, detailing which fields the query works on. See FT.SEARCH for API details
 */
t_fieldMask IndexSpec_ParseFieldMask(IndexSpec *sp, RedisModuleString **argv, int argc);

void IndexSpec_InitializeSynonym(IndexSpec *sp);

#ifdef __cplusplus
}
#endif
#endif
