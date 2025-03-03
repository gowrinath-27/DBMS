#include "OpenRelTable.h"

#include <cstring>
#include <cstdlib>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    tableMetaInfo[i].free = true;
  }

  /************ Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/

  // set up the relation cache entry for the attribute catalog similarly
  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
  RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
  relCacheEntry.recId.block=RELCAT_BLOCK;
  relCacheEntry.recId.slot=RELCAT_SLOTNUM_FOR_ATTRCAT;

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]
  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID])=relCacheEntry;

  /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);

  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

  // iterate through all the attributes of the relation catalog and create a linked
  // list of AttrCacheEntry (slots 0 to 5)
  // for each of the entries, set
  //    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
  //    attrCacheEntry.recId.slot = i   (0 to 5)
  //    and attrCacheEntry.next appropriately
  // NOTE: allocate each entry dynamically using malloc
  AttrCacheEntry *head=nullptr;
  AttrCacheEntry *prev=nullptr;
  for(int i=0;i<RELCAT_NO_ATTRS;i++)
  {
    attrCatBlock.getRecord(attrCatRecord,i);
    AttrCacheEntry *attrCacheEntry=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&(attrCacheEntry->attrCatEntry));
    attrCacheEntry->recId.block=ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot=i;
    attrCacheEntry->next=nullptr;
    if(head==nullptr)
      head=attrCacheEntry;
    else
      prev->next=attrCacheEntry;
    prev=attrCacheEntry;
  }

  // set the next field in the last entry to nullptr

  AttrCacheTable::attrCache[RELCAT_RELID] = head; /* head of the linked list */

  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

  // set up the attributes of the attribute cache similarly.
  // read slots 6-11 from attrCatBlock and initialise recId appropriately
  head=nullptr;
  prev=nullptr;
  for(int i=RELCAT_NO_ATTRS;i<RELCAT_NO_ATTRS+ATTRCAT_NO_ATTRS;i++)
  {
    attrCatBlock.getRecord(attrCatRecord,i);
    AttrCacheEntry *attrCacheEntry=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&(attrCacheEntry->attrCatEntry));
    attrCacheEntry->recId.block=ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot=i;
    attrCacheEntry->next=nullptr;
    if(head==nullptr)
      head=attrCacheEntry;
    else
      prev->next=attrCacheEntry;
    prev=attrCacheEntry;
  }

  // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
  AttrCacheTable::attrCache[ATTRCAT_RELID]=head;

  /************ Setting up tableMetaInfo entries ************/

  // in the tableMetaInfo array
  //   set free = false for RELCAT_RELID and ATTRCAT_RELID
  tableMetaInfo[RELCAT_RELID].free = false;
  strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);
  tableMetaInfo[ATTRCAT_RELID].free = false;
  strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
  //   set relname for RELCAT_RELID and ATTRCAT_RELID
}


int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

  /* traverse through the tableMetaInfo array,
    find the entry in the Open Relation Table corresponding to relName.*/
  for(int i=0; i < MAX_OPEN; i++)
  {
    if(tableMetaInfo[i].free == false && strcmp(tableMetaInfo[i].relName,relName)==0)
      return i;
  }
  return E_RELNOTOPEN;

  // if found return the relation id, else indicate that the relation do not
  // have an entry in the Open Relation Table.
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {

  if(OpenRelTable::getRelId(relName)!=E_RELNOTOPEN){
    // (checked using OpenRelTable::getRelId())

    // return that relation id;
    return OpenRelTable::getRelId(relName);
  }

  /* find a free slot in the Open Relation Table
     using OpenRelTable::getFreeOpenRelTableEntry(). */
  int free_slot = OpenRelTable::getFreeOpenRelTableEntry();

  if (free_slot == E_CACHEFULL){
    return E_CACHEFULL;
  }

  // let relId be used to store the free slot.
  int relId;
  relId = free_slot;

  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/

  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  Attribute buf;
  strcpy(buf.sVal,relName);
  char str[ATTR_SIZE]="RelName";
  RelCacheTable::resetSearchIndex(RELCAT_RELID);
  RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID,str,buf,EQ);

  if (relcatRecId.block == -1 && relcatRecId.slot == -1) {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

  /* read the record entry corresponding to relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
      update the recId field of this Relation Cache entry to relcatRecId.
      use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */
  RecBuffer relCatBlock(relcatRecId.block);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, relcatRecId.slot);

  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId = relcatRecId;
  // relCacheEntry.dirty = true;
  RelCacheTable::relCache[relId] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[relId]) = relCacheEntry;

  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  AttrCacheEntry* listHead=nullptr;
  AttrCacheEntry* prev=nullptr;
  /*iterate over all the entries in the Attribute Catalog corresponding to each
  attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
  care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
  corresponding to Attribute Catalog before the first call to linearSearch().*/
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
  while(true)
  {
      /* let attrcatRecId store a valid record id an entry of the relation, relName,
      in the Attribute Catalog.*/

      Attribute bu;
      strcpy(bu.sVal,relName);
      char str[ATTR_SIZE]="RelName";
      RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID,str,bu,EQ);
      if(attrcatRecId.block == -1 && attrcatRecId.slot == -1)
        break;
      RecBuffer buffer(attrcatRecId.block);
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      buffer.getRecord(attrCatRecord,attrcatRecId.slot);

      AttrCacheEntry *attrCacheEntry=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
      AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&(attrCacheEntry->attrCatEntry));
      attrCacheEntry->recId.block=attrcatRecId.block;
      attrCacheEntry->recId.slot=attrcatRecId.slot;
      // attrCacheEntry->dirty = true;
      attrCacheEntry->next=nullptr;
      if(listHead == nullptr)
      {
        listHead = attrCacheEntry;
      }
      else
        prev->next=attrCacheEntry;
      prev = attrCacheEntry;

      /* read the record entry corresponding to attrcatRecId and create an
      Attribute Cache entry on it using RecBuffer::getRecord() and
      AttrCacheTable::recordToAttrCatEntry().
      update the recId field of this Attribute Cache entry to attrcatRecId.
      add the Attribute Cache entry to the linked list of listHead .*/
      // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
  }

  // set the relIdth entry of the AttrCacheTable to listHead.
  AttrCacheTable::attrCache[relId]=(struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
  AttrCacheTable::attrCache[relId]=listHead;
  /****** Setting up metadata in the Open Relation Table for the relation******/

  // update the relIdth entry of the tableMetaInfo with free as false and
  // relName as the input.
  tableMetaInfo[relId].free = false;
  strcpy(tableMetaInfo[relId].relName,relName);

  return relId;
}


int OpenRelTable::closeRel(int relId) {
  if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
    return E_NOTPERMITTED;
  }

  if (relId < 0 || relId >=MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free == true) {
    return E_RELNOTOPEN;
  }

  if(RelCacheTable::relCache[relId]->dirty == true)
  {
    RelCatEntry relCatEntry = RelCacheTable::relCache[relId]->relCatEntry;
    Attribute record[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&relCatEntry,record);
    char str[ATTR_SIZE] = RELCAT_ATTR_RELNAME;
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID,str,record[RELCAT_REL_NAME_INDEX],EQ);
    RecBuffer relCatBlock(recId.block);
    relCatBlock.setRecord(record,recId.slot);
  }

  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function

  if(RelCacheTable::relCache[relId]!=nullptr)
     delete(RelCacheTable::relCache[relId]);

  AttrCacheEntry* next=nullptr;
  for(AttrCacheEntry* attrCacheEntry= AttrCacheTable::attrCache[relId];attrCacheEntry!=nullptr;attrCacheEntry=next)
  {
      if(attrCacheEntry->dirty == true)
      {
        AttrCatEntry attrCatEntry = attrCacheEntry->attrCatEntry;
        Attribute record[ATTRCAT_NO_ATTRS];
        AttrCacheTable::attrCatEntryToRecord(&attrCatEntry,record);
        RecBuffer attrCatBlock(attrCacheEntry->recId.block);
        attrCatBlock.setRecord(record, attrCacheEntry->recId.slot);
      }
      next=attrCacheEntry->next;
      delete(attrCacheEntry);
      attrCacheEntry = nullptr;
  }
  // update `tableMetaInfo` to set `relId` as a free slot
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
  tableMetaInfo[relId].free = true;
  RelCacheTable::relCache[relId]=nullptr;
  AttrCacheTable::attrCache[relId]=nullptr;

  return SUCCESS;
}


int OpenRelTable::getFreeOpenRelTableEntry() {

  /* traverse through the tableMetaInfo array,
    find a free entry in the Open Relation Table.*/
  for(int i = 0;i < MAX_OPEN;i++)
  {
    if(tableMetaInfo[i].free == true)
      return i;
  }
  return E_CACHEFULL;
  // if found return the relation id, else return E_CACHEFULL.
}

OpenRelTable::~OpenRelTable() {
  // close all open relations (from rel-id = 2 onwards. Why?)
  for (int i = 2; i < MAX_OPEN; ++i) {
    if (tableMetaInfo[i].free == false) {
      OpenRelTable::closeRel(i); // we will implement this function later
    }
  }

  /**** Closing the catalog relations in the relation cache ****/

    //releasing the relation cache entry of the attribute catalog

    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty == true)
    {

        /* Get the Relation Catalog entry from RelCacheTable::relCache
        Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(ATTRCAT_RELID,&relCatEntry);
        Attribute record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatEntry,record);
        // declaring an object of RecBuffer class to write back to the buffer
        RelCacheTable::resetSearchIndex(RELCAT_RELID);
        char str[ATTR_SIZE] = RELCAT_ATTR_RELNAME;
        RecId recId = BlockAccess::linearSearch(RELCAT_RELID,str,record[RELCAT_REL_NAME_INDEX],EQ);
        RecBuffer relCatBlock(recId.block);
        relCatBlock.setRecord(record,recId.slot);
        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
    }

    //releasing the relation cache entry of the relation catalog

    if(RelCacheTable::relCache[RELCAT_RELID]->dirty == true) {

        /* Get the Relation Catalog entry from RelCacheTable::relCache
        Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
        RelCatEntry relCatEntry = RelCacheTable::relCache[RELCAT_RELID]->relCatEntry;
        Attribute record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatEntry,record);

        // declaring an object of RecBuffer class to write back to the buffer
        RelCacheTable::resetSearchIndex(RELCAT_RELID);
        char str[ATTR_SIZE] = RELCAT_ATTR_RELNAME;
        RecId recId = BlockAccess::linearSearch(RELCAT_RELID,str,record[RELCAT_REL_NAME_INDEX],EQ);
        RecBuffer relCatBlock(recId.block);
        relCatBlock.setRecord(record,recId.slot);
        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
    }
    // free the memory dynamically allocated to this RelCacheEntry
    delete(RelCacheTable::relCache[ATTRCAT_RELID]);

    // free the memory dynamically allocated for this RelCacheEntry
    delete(RelCacheTable::relCache[RELCAT_RELID]);

    // free the memory allocated for the attribute cache entries of the
    // relation catalog and the attribute catalog
    delete(AttrCacheTable::attrCache[RELCAT_RELID]);
    delete(AttrCacheTable::attrCache[ATTRCAT_RELID]);

}

