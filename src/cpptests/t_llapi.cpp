#include "../redisearch_api.h"
#include <gtest/gtest.h>

#define DOCID1 "doc1"
#define DOCID2 "doc2"
#define FIELD_NAME_1 "text1"
#define FIELD_NAME_2 "text2"
#define NUMERIC_FIELD_NAME "num"
#define TAG_FIELD_NAME1 "tag1"
#define TAG_FIELD_NAME2 "tag2"

class LLApiTest : public ::testing::Test {
  virtual void SetUp() {
    RediSearch_Initialize();
  }

  virtual void TearDown() {
  }
};

TEST_F(LLApiTest, testGetVersion) {
  ASSERT_EQ(RediSearch_GetLowLevelApiVersion(), REDISEARCH_LOW_LEVEL_API_VERSION);
}

TEST_F(LLApiTest, testAddDocumetTextField) {
  // creating the index
  Index* index = RediSearch_CreateSpec("index", NULL, NULL);

  // adding text field to the index
  RediSearch_CreateTextField(index, FIELD_NAME_1);

  // adding document to the index
  Doc* d = RediSearch_CreateDocument(DOCID1, strlen(DOCID1), 1.0, NULL);
  RediSearch_DocumentAddTextField(d, FIELD_NAME_1, "some test to index");
  RediSearch_SpecAddDocument(index, d);

  // searching on the index
#define SEARCH_TERM "index"
  QN* qn = RediSearch_CreateTokenNode(index, FIELD_NAME_1, SEARCH_TERM);
  ResultsIterator* iter = RediSearch_GetResutlsIterator(qn, index);

  size_t len;
  const char* id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResutlsIteratorFree(iter);

  // test prefix search
  qn = RediSearch_CreatePrefixNode(index, FIELD_NAME_1, "in");
  iter = RediSearch_GetResutlsIterator(qn, index);

  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResutlsIteratorFree(iter);

  // search with no results
  qn = RediSearch_CreatePrefixNode(index, FIELD_NAME_1, "nn");
  iter = RediSearch_GetResutlsIterator(qn, index);
  ASSERT_FALSE(iter);

  // adding another text field
  RediSearch_CreateTextField(index, FIELD_NAME_2);

  // adding document to the index with both fields
  d = RediSearch_CreateDocument(DOCID2, strlen(DOCID2), 1.0, NULL);
  RediSearch_DocumentAddTextField(d, FIELD_NAME_1, "another indexing testing");
  RediSearch_DocumentAddTextField(d, FIELD_NAME_2, "another indexing testing");
  RediSearch_SpecAddDocument(index, d);

  // test prefix search, should return both documents now
  qn = RediSearch_CreatePrefixNode(index, FIELD_NAME_1, "in");
  iter = RediSearch_GetResutlsIterator(qn, index);

  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID2);
  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResutlsIteratorFree(iter);

  // test prefix search on second field, should return only second document
  qn = RediSearch_CreatePrefixNode(index, FIELD_NAME_2, "an");
  iter = RediSearch_GetResutlsIterator(qn, index);

  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID2);
  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResutlsIteratorFree(iter);

  // delete the second document
  int ret = RediSearch_DropDocument(index, DOCID2, strlen(DOCID2));
  ASSERT_TRUE(ret);

  // searching again, make sure there is no results
  qn = RediSearch_CreatePrefixNode(index, FIELD_NAME_2, "an");
  iter = RediSearch_GetResutlsIterator(qn, index);

  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResutlsIteratorFree(iter);
}

TEST_F(LLApiTest, testAddDocumetNumericField) {
  // creating the index
  Index* index = RediSearch_CreateSpec("index", NULL, NULL);

  // adding text field to the index
  RediSearch_CreateNumericField(index, NUMERIC_FIELD_NAME);

  // adding document to the index
  Doc* d = RediSearch_CreateDocument(DOCID1, strlen(DOCID1), 1.0, NULL);
  RediSearch_DocumentAddNumericField(d, NUMERIC_FIELD_NAME, 20);
  RediSearch_SpecAddDocument(index, d);

  // searching on the index
  QN* qn = RediSearch_CreateNumericNode(index, NUMERIC_FIELD_NAME, 30, 10, 0, 0);
  ResultsIterator* iter = RediSearch_GetResutlsIterator(qn, index);

  size_t len;
  const char* id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResutlsIteratorFree(iter);
}

TEST_F(LLApiTest, testAddDocumetTagField) {
  // creating the index
  Index* index = RediSearch_CreateSpec("index", NULL, NULL);

  // adding text field to the index
  RediSearch_CreateTagField(index, TAG_FIELD_NAME1);

  // adding document to the index
#define TAG_VALUE "tag_value"
  Doc* d = RediSearch_CreateDocument(DOCID1, strlen(DOCID1), 1.0, NULL);
  RediSearch_DocumentAddTextField(d, TAG_FIELD_NAME1, TAG_VALUE);
  RediSearch_SpecAddDocument(index, d);

  // searching on the index
  QN* qn = RediSearch_CreateTagNode(index, TAG_FIELD_NAME1);
  QN* tqn = RediSearch_CreateTokenNode(index, NULL, TAG_VALUE);
  RediSearch_TagNodeAddChild(qn, tqn);
  ResultsIterator* iter = RediSearch_GetResutlsIterator(qn, index);

  size_t len;
  const char* id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResutlsIteratorFree(iter);

  // prefix search on the index
  qn = RediSearch_CreateTagNode(index, TAG_FIELD_NAME1);
  tqn = RediSearch_CreatePrefixNode(index, NULL, "ta");
  RediSearch_TagNodeAddChild(qn, tqn);
  iter = RediSearch_GetResutlsIterator(qn, index);

  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResutlsIteratorFree(iter);
}

TEST_F(LLApiTest, testPhoneticSearch) {
  // creating the index
  Index* index = RediSearch_CreateSpec("index", NULL, NULL);
  Field* f = RediSearch_CreateTextField(index, FIELD_NAME_1);
  RediSearch_TextFieldPhonetic(f, index);

  // creating none phonetic field
  RediSearch_CreateTextField(index, FIELD_NAME_2);

  Doc* d = RediSearch_CreateDocument(DOCID1, strlen(DOCID1), 1.0, NULL);
  RediSearch_DocumentAddTextField(d, FIELD_NAME_1, "felix");
  RediSearch_DocumentAddTextField(d, FIELD_NAME_2, "felix");
  RediSearch_SpecAddDocument(index, d);

  // make sure phonetic search works on field1
  QN* qn = RediSearch_CreateTokenNode(index, FIELD_NAME_1, "phelix");
  ResultsIterator* iter = RediSearch_GetResutlsIterator(qn, index);

  size_t len;
  const char* id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResutlsIteratorFree(iter);

  // make sure phonetic search on field2 do not return results
  qn = RediSearch_CreateTokenNode(index, FIELD_NAME_2, "phelix");
  iter = RediSearch_GetResutlsIterator(qn, index);
  ASSERT_FALSE(iter);
}

TEST_F(LLApiTest, testMassivePrefix) {
  // creating the index
  Index* index = RediSearch_CreateSpec("index", NULL, NULL);
  RediSearch_CreateTagField(index, TAG_FIELD_NAME1);

  char buff[1024];
  int NUM_OF_DOCS = 1000;
  for (int i = 0; i < NUM_OF_DOCS; ++i) {
    sprintf(buff, "doc%d", i);
    Doc* d = RediSearch_CreateDocument(buff, strlen(buff), 1.0, NULL);
    sprintf(buff, "tag-%d", i);
    RediSearch_DocumentAddTextField(d, TAG_FIELD_NAME1, buff);
    RediSearch_SpecAddDocument(index, d);
  }

  QN* qn = RediSearch_CreateTagNode(index, TAG_FIELD_NAME1);
  QN* pqn = RediSearch_CreatePrefixNode(index, NULL, "tag-");
  RediSearch_TagNodeAddChild(qn, pqn);
  ResultsIterator* iter = RediSearch_GetResutlsIterator(qn, index);
  ASSERT_TRUE(iter);

  for (size_t i = 0; i < NUM_OF_DOCS; ++i) {
    size_t len;
    const char* id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
    ASSERT_TRUE(id);
  }

  RediSearch_ResutlsIteratorFree(iter);
}

char buffer[1024];

int GetValue(void* ctx, const char* fieldName, const void* id, char** strVal, double* doubleVal) {
  *strVal = buffer;
  int numId;
  sscanf((char*)id, "doc%d", &numId);
  if (strcmp(fieldName, TAG_FIELD_NAME1) == 0) {
    sprintf(*strVal, "tag1-%d", numId);
  } else {
    sprintf(*strVal, "tag2-%d", numId);
  }
  return STR_VALUE_TYPE;
}

TEST_F(LLApiTest, testMassivePrefixWithUnsortedSupport) {
  // creating the index
  Index* index = RediSearch_CreateSpec("index", GetValue, NULL);
  RediSearch_CreateTagField(index, TAG_FIELD_NAME1);

  char buff[1024];
  int NUM_OF_DOCS = 10000;
  for (int i = 0; i < NUM_OF_DOCS; ++i) {
    sprintf(buff, "doc%d", i);
    Doc* d = RediSearch_CreateDocument(buff, strlen(buff), 1.0, NULL);
    sprintf(buff, "tag-%d", i);
    RediSearch_DocumentAddTextField(d, TAG_FIELD_NAME1, buff);
    RediSearch_SpecAddDocument(index, d);
  }

  QN* qn = RediSearch_CreateTagNode(index, TAG_FIELD_NAME1);
  QN* pqn = RediSearch_CreatePrefixNode(index, NULL, "tag-");
  RediSearch_TagNodeAddChild(qn, pqn);
  ResultsIterator* iter = RediSearch_GetResutlsIterator(qn, index);
  ASSERT_TRUE(iter);

  for (size_t i = 0; i < NUM_OF_DOCS; ++i) {
    size_t len;
    const char* id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
    ASSERT_TRUE(id);
  }

  RediSearch_ResutlsIteratorFree(iter);
}

TEST_F(LLApiTest, testPrefixIntersection) {
  // creating the index
  Index* index = RediSearch_CreateSpec("index", GetValue, NULL);
  RediSearch_CreateTagField(index, TAG_FIELD_NAME1);
  RediSearch_CreateTagField(index, TAG_FIELD_NAME2);

  char buff[1024];
  int NUM_OF_DOCS = 1000;
  for (int i = 0; i < NUM_OF_DOCS; ++i) {
    sprintf(buff, "doc%d", i);
    Doc* d = RediSearch_CreateDocument(buff, strlen(buff), 1.0, NULL);
    sprintf(buff, "tag1-%d", i);
    RediSearch_DocumentAddTextField(d, TAG_FIELD_NAME1, buff);
    sprintf(buff, "tag2-%d", i);
    RediSearch_DocumentAddTextField(d, TAG_FIELD_NAME2, buff);
    RediSearch_SpecAddDocument(index, d);
  }

  QN* qn1 = RediSearch_CreateTagNode(index, TAG_FIELD_NAME1);
  QN* pqn1 = RediSearch_CreatePrefixNode(index, NULL, "tag1-");
  RediSearch_TagNodeAddChild(qn1, pqn1);
  QN* qn2 = RediSearch_CreateTagNode(index, TAG_FIELD_NAME2);
  QN* pqn2 = RediSearch_CreatePrefixNode(index, NULL, "tag2-");
  RediSearch_TagNodeAddChild(qn2, pqn2);
  QN* iqn = RediSearch_CreateIntersectNode(index, 0);
  RediSearch_IntersectNodeAddChild(iqn, qn1);
  RediSearch_IntersectNodeAddChild(iqn, qn2);

  ResultsIterator* iter = RediSearch_GetResutlsIterator(iqn, index);
  ASSERT_TRUE(iter);

  for (size_t i = 0; i < NUM_OF_DOCS; ++i) {
    size_t len;
    const char* id = (const char*)RediSearch_ResutlsIteratorNext(iter, index, &len);
    ASSERT_STRNE(id, NULL);
  }

  RediSearch_ResutlsIteratorFree(iter);
}
