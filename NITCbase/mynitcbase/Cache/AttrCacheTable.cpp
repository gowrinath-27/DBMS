#include "AttrCacheTable.h"

#include <cstring>

AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];

/* returns the attrOffset-th attribute for the relation corresponding to relId
NOTE: this function expects the caller to allocate memory for `*attrCatBuf`
*/
int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCatBuf) {
  // check if 0 <= relId < MAX_OPEN and return E_OUTOFBOUND otherwise
    if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  // check if attrCache[relId] == nullptr and return E_RELNOTOPEN if true
  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  // traverse the linked list of attribute cache entries
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (entry->attrCatEntry.offset == attrOffset)
    {
      (*attrCatBuf)=entry->attrCatEntry;
      // copy entry->attrCatEntry to *attrCatBuf and return SUCCESS
      return SUCCESS;
    }
  }

  // there is no attribute at this offset
  return E_ATTRNOTEXIST;
}

/* Converts a attribute catalog record to AttrCatEntry struct
    We get the record as Attribute[] from the BlockBuffer.getRecord() function.
    This function will convert that to a struct AttrCatEntry type.
*/
void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS],AttrCatEntry* attrCatEntry) {
  strcpy(attrCatEntry->relName, record[ATTRCAT_REL_NAME_INDEX].sVal);
  strcpy(attrCatEntry->attrName, record[ATTRCAT_ATTR_NAME_INDEX].sVal);
  attrCatEntry->attrType = (int)record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
  if(record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal == -1)
    attrCatEntry->primaryFlag = false;
  else
    attrCatEntry->primaryFlag = true;
  attrCatEntry->rootBlock = (int)record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
  attrCatEntry->offset = (int)record[ATTRCAT_OFFSET_INDEX].nVal;

  // copy the rest of the fields in the record to the attrCacheEntry struct
}

void AttrCacheTable::attrCatEntryToRecord(AttrCatEntry* attrCatEntry,
                                          union Attribute record[ATTRCAT_NO_ATTRS]) {
  strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal, attrCatEntry->relName);
  strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal, attrCatEntry->attrName);
  record[ATTRCAT_ATTR_TYPE_INDEX].nVal = (int)attrCatEntry->attrType;
  if(attrCatEntry->primaryFlag == true)
    record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = 1;
  else
    record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = -1;
  record[ATTRCAT_ROOT_BLOCK_INDEX].nVal = (int)attrCatEntry->rootBlock;
  record[ATTRCAT_OFFSET_INDEX].nVal = (int)attrCatEntry->offset;

}

/* returns the attribute with name `attrName` for the relation corresponding to relId
NOTE: this function expects the caller to allocate memory for `*attrCatBuf`
*/
int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry* attrCatBuf) {

  // check that relId is valid and corresponds to an open relation
    if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  // iterate over the entries in the attribute cache and set attrCatBuf to the entry that
  //    matches attrName
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (strcmp(entry->attrCatEntry.attrName,attrName) == 0)
    {
      (*attrCatBuf)=entry->attrCatEntry;
      // copy entry->attrCatEntry to *attrCatBuf and return SUCCESS
      return SUCCESS;
    }
  }

  // no attribute with name attrName for the relation
  return E_ATTRNOTEXIST;
}

int AttrCacheTable::getSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) {

  if(relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry *entry = attrCache[relId];entry!=nullptr;entry=entry->next)
  {
    if (strcmp(entry->attrCatEntry.attrName,attrName) == 0)
    {
      //copy the searchIndex field of the corresponding Attribute Cache entry
      //in the Attribute Cache Table to input searchIndex variable.
      *searchIndex = entry->searchIndex;

      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::getSearchIndex(int relId, int attrOffset, IndexId *searchIndex) {

  if(relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry *entry = attrCache[relId];entry!=nullptr;entry=entry->next)
  {
    if (entry->attrCatEntry.offset == attrOffset)
    {
      //copy the searchIndex field of the corresponding Attribute Cache entry
      //in the Attribute Cache Table to input searchIndex variable.
      *searchIndex = entry->searchIndex;

      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::setSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) {

  if(relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry *entry = attrCache[relId];entry!=nullptr;entry=entry->next)
  {
    if (strcmp(entry->attrCatEntry.attrName,attrName) == 0)
    {
      // copy the input searchIndex variable to the searchIndex field of the
      //corresponding Attribute Cache entry in the Attribute Cache Table.
      entry->searchIndex = *searchIndex;

      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::setSearchIndex(int relId, int attrOffset, IndexId *searchIndex) {

  if(relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry *entry = attrCache[relId];entry!=nullptr;entry=entry->next)
  {
    if (entry->attrCatEntry.offset == attrOffset)
    {
      //copy the searchIndex field of the corresponding Attribute Cache entry
      //in the Attribute Cache Table to input searchIndex variable.
      entry->searchIndex = *searchIndex;

      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::resetSearchIndex(int relId, char attrName[ATTR_SIZE]) {

  // declare an IndexId having value {-1, -1}
  IndexId searchindex = {-1,-1};
  // set the search index to {-1, -1} using AttrCacheTable::setSearchIndex
  return setSearchIndex(relId, attrName, &searchindex);
  // return the value returned by setSearchIndex
}

int AttrCacheTable::resetSearchIndex(int relId, int attrOffset) {

  // declare an IndexId having value {-1, -1}
  IndexId searchindex = {-1,-1};
  // set the search index to {-1, -1} using AttrCacheTable::setSearchIndex
  return setSearchIndex(relId, attrOffset, &searchindex);
  // return the value returned by setSearchIndex
}

int AttrCacheTable::setAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry *attrCatBuf) {

  if(relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry *entry = attrCache[relId];entry!=nullptr;entry=entry->next)
  {
    if(strcmp(entry->attrCatEntry.attrName,attrName) == 0)
    {
      // copy the attrCatBuf to the corresponding Attribute Catalog entry in
      // the Attribute Cache Table.
      entry->attrCatEntry = *attrCatBuf;

      // set the dirty flag of the corresponding Attribute Cache entry in the
      // Attribute Cache Table.
      entry->dirty = true;

      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::setAttrCatEntry(int relId, int attrOffset, AttrCatEntry *attrCatBuf) {

  if(relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry *entry = attrCache[relId];entry!=nullptr;entry=entry->next)
  {
    if(entry->attrCatEntry.offset == attrOffset)
    {
      // copy the attrCatBuf to the corresponding Attribute Catalog entry in
      // the Attribute Cache Table.
      entry->attrCatEntry = *attrCatBuf;

      // set the dirty flag of the corresponding Attribute Cache entry in the
      // Attribute Cache Table.
      entry->dirty = true;

      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}
