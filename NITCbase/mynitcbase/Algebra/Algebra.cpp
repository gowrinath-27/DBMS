#include "Algebra.h"
#include<cstdio>
#include<cstdlib>
#include <cstring>

// will return if a string can be parsed as a floating point number
bool isNumber(char *str) {
  int len;
  float ignore;
  /*
    sscanf returns the number of elements read, so if there is no float matching
    the first %f, ret will be 0, else it'll be 1

    %n gets the number of characters read. this scanf sequence will read the
    first float ignoring all the whitespace before and after. and the number of
    characters read that far will be stored in len. if len == strlen(str), then
    the string only contains a float with/without whitespace. else, there's other
    characters.
  */
  int ret = sscanf(str, "%f %n", &ignore, &len);
  return ret == 1 && len == strlen(str);
}


/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE])
{
  int srcRelId = OpenRelTable::getRelId(srcRel);      // we'll implement this later
  if (srcRelId<0 || srcRelId>=MAX_OPEN) {
    return E_RELNOTOPEN;
  }

  AttrCatEntry attrCatEntry;
  // get the attribute catalog entry for attr, using AttrCacheTable::getAttrcatEntry()
  //    return E_ATTRNOTEXIST if it returns the error
  int ret = AttrCacheTable::getAttrCatEntry(srcRelId,attr,&attrCatEntry);
  if(ret != SUCCESS)
      return E_ATTRNOTEXIST;


  /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/
  int type = attrCatEntry.attrType;
  Attribute attrVal;
  if (type == NUMBER)
  {
    if (isNumber(strVal))
    {
      // the isNumber() function is implemented below
      attrVal.nVal = atof(strVal);
    }
    else
    {
      return E_ATTRTYPEMISMATCH;
    }
  }
  else if (type == STRING)
  {
    strcpy(attrVal.sVal, strVal);
  }

  /*** Creating and opening the target relation ***/
    // Prepare arguments for createRel() in the following way:
    // get RelcatEntry of srcRel using RelCacheTable::getRelCatEntry()
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatEntry);
    int src_nAttrs = relCatEntry.numAttrs;/* the no. of attributes present in src relation */


    /* let attr_names[src_nAttrs][ATTR_SIZE] be a 2D array of type char
        (will store the attribute names of rel). */
    char attr_names[src_nAttrs][ATTR_SIZE];
    // let attr_types[src_nAttrs] be an array of type int
    int attr_types[src_nAttrs];

    /*iterate through 0 to src_nAttrs-1 :
        get the i'th attribute's AttrCatEntry using AttrCacheTable::getAttrCatEntry()
        fill the attr_names, attr_types arrays that we declared with the entries
        of corresponding attributes
    */
    for(int i=0;i<src_nAttrs;i++)
    {
      AttrCatEntry attrEntry;
      AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrEntry);
      strcpy(attr_names[i],attrEntry.attrName);
      attr_types[i] = attrEntry.attrType;
    }


    /* Create the relation for target relation by calling Schema::createRel()
       by providing appropriate arguments */
    ret = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
    // if the createRel returns an error code, then return that value.
    if(ret != SUCCESS)
      return ret;

    /* Open the newly created target relation by calling OpenRelTable::openRel()
       method and store the target relid */
    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId<0 || targetRelId>=MAX_OPEN)
    {
      Schema::deleteRel(targetRel);
      return targetRelId;
    }
    /* If opening fails, delete the target relation by calling Schema::deleteRel()
       and return the error value returned from openRel() */

    /*** Selecting and inserting records into the target relation ***/
    /* Before calling the search function, reset the search to start from the
       first using RelCacheTable::resetSearchIndex() */

    Attribute record[src_nAttrs];

    /*
        The BlockAccess::search() function can either do a linearSearch or
        a B+ tree search. Hence, reset the search index of the relation in the
        relation cache using RelCacheTable::resetSearchIndex().
        Also, reset the search index in the attribute cache for the select
        condition attribute with name given by the argument `attr`. Use
        AttrCacheTable::resetSearchIndex().
        Both these calls are necessary to ensure that search begins from the
        first record.
    */
    RelCacheTable::resetSearchIndex(srcRelId);
    // AttrCacheTable::resetSearchIndex(/* fill arguments */);
    AttrCacheTable::resetSearchIndex(srcRelId, attr);

    // read every record that satisfies the condition by repeatedly calling
    // BlockAccess::search() until there are no more records to be read

    while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS) {

        ret = BlockAccess::insert(targetRelId, record);

        if (ret != SUCCESS)
        {
        //     close the targetrel(by calling Schema::closeRel(targetrel))
          Schema::closeRel(targetRel);
        //     delete targetrel (by calling Schema::deleteRel(targetrel))
          Schema::deleteRel(targetRel);
        //     return ret;
          return ret;
        }
    }

    // Close the targetRel by calling closeRel() method of schema layer
    Schema::closeRel(targetRel);
    // return SUCCESS.
    return SUCCESS;
}

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]){
    // if relName is equal to "RELATIONCAT" or "ATTRIBUTECAT"
    // return E_NOTPERMITTED;
  if(strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0)
  {
    return E_NOTPERMITTED;
  }

    // get the relation's rel-id using OpenRelTable::getRelId() method
    int relId = OpenRelTable::getRelId(relName);

    // if relation is not open in open relation table, return E_RELNOTOPEN
    // (check if the value returned from getRelId function call = E_RELNOTOPEN)
    if(relId == E_RELNOTOPEN)
      return relId;
    // get the relation catalog entry from relation cache
    // (use RelCacheTable::getRelCatEntry() of Cache Layer)
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);

    /* if relCatEntry.numAttrs != numberOfAttributes in relation,
       return E_NATTRMISMATCH */
    if(relCatEntry.numAttrs != nAttrs)
      return E_NATTRMISMATCH;

    // let recordValues[numberOfAttributes] be an array of type union Attribute
    Attribute recordValues[nAttrs];
    /*
        Converting 2D char array of record values to Attribute array recordValues
     */
    // iterate through 0 to nAttrs-1: (let i be the iterator)
    for(int i=0;i<nAttrs;i++)
    {
        // get the attr-cat entry for the i'th attribute from the attr-cache
        // (use AttrCacheTable::getAttrCatEntry())
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId,i,&attrCatEntry);

        // let type = attrCatEntry.attrType;
        int type=attrCatEntry.attrType;

        if (type == NUMBER)
        {
            // if the char array record[i] can be converted to a number
            // (check this using isNumber() function)
            if(isNumber(record[i]))
            {
                /* convert the char array to numeral and store it
                   at recordValues[i].nVal using atof() */
                recordValues[i].nVal = atof(record[i]);
            }
            else
            {
                return E_ATTRTYPEMISMATCH;
            }
        }
        else if (type == STRING)
        {
            // copy record[i] to recordValues[i].sVal
          strcpy(recordValues[i].sVal,record[i]);
        }
    }

    // insert the record by calling BlockAccess::insert() function
    // let retVal denote the return value of insert call
    int retVal = BlockAccess::insert(relId,recordValues);

    return retVal;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {

    int srcRelId = OpenRelTable::getRelId(srcRel);/*srcRel's rel-id (use OpenRelTable::getRelId() function)*/

    // if srcRel is not open in open relation table, return E_RELNOTOPEN
    if(srcRelId<0 || srcRelId>=MAX_OPEN)
      return srcRelId;

    // get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    // get the no. of attributes present in relation from the fetched RelCatEntry.
    int numAttrs = relCatEntry.numAttrs;

    // attrNames and attrTypes will be used to store the attribute names
    // and types of the source relation respectively
    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];

    /*iterate through every attribute of the source relation :
        - get the AttributeCat entry of the attribute with offset.
          (using AttrCacheTable::getAttrCatEntry())
        - fill the arrays attrNames and attrTypes that we declared earlier
          with the data about each attribute
    */
    for(int i=0;i<numAttrs;i++)
    {
      AttrCatEntry attrCatEntry;
      AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
      strcpy(attrNames[i], attrCatEntry.attrName);
      attrTypes[i] = attrCatEntry.attrType;
    }


    /* Creating and opening the target relation */

    // Create a relation for target relation by calling Schema::createRel()
    int ret = Schema::createRel(targetRel, numAttrs, attrNames, attrTypes);

    // if the createRel returns an error code, then return that value.
    if(ret != SUCCESS)
      return ret;
    // Open the newly created target relation by calling OpenRelTable::openRel()
    // and get the target relid

    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId<0 || targetRelId>=MAX_OPEN)
    {
      Schema::deleteRel(targetRel);
      return targetRelId;
    }
    // If opening fails, delete the target relation by calling Schema::deleteRel() of
    // return the error value returned from openRel().


    /* Inserting projected records into the target relation */

    // Take care to reset the searchIndex before calling the project function
    // using RelCacheTable::resetSearchIndex()
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[numAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS)
    {
        // record will contain the next record

        ret = BlockAccess::insert(targetRelId, record);

        if (ret != SUCCESS)
        {
            // close the targetrel by calling Schema::closeRel()
            Schema::closeRel(targetRel);
            // delete targetrel by calling Schema::deleteRel()
            Schema::deleteRel(targetRel);
            // return ret
            return ret;
        }
    }

    // Close the targetRel by calling Schema::closeRel()
    Schema::closeRel(targetRel);

    // return SUCCESS.
    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {

    int srcRelId = OpenRelTable::getRelId(srcRel);/*srcRel's rel-id (use OpenRelTable::getRelId() function)*/
    if(srcRelId<0 || srcRelId>=MAX_OPEN)
      return srcRelId;

    // if srcRel is not open in open relation table, return E_RELNOTOPEN

    // get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    // get the no. of attributes present in relation from the fetched RelCatEntry.
    int src_nAttrs = relCatEntry.numAttrs;

    // declare attr_offset[tar_nAttrs] an array of type int.
    int attr_offset[tar_nAttrs];
    // where i-th entry will store the offset in a record of srcRel for the
    // i-th attribute in the target relation.

    // let attr_types[tar_nAttrs] be an array of type int.
    int attr_types[tar_nAttrs];
    // where i-th entry will store the type of the i-th attribute in the
    // target relation.


    /* Checking if attributes of target are present in the source relation
         and storing its offsets and types */

    /*iterate through 0 to tar_nAttrs-1 :
        - get the attribute catalog entry of the attribute with name tar_attrs[i].
        - if the attribute is not found return E_ATTRNOTEXIST
        - fill the attr_offset, attr_types arrays of target relation from the
          corresponding attribute catalog entries of source relation
    */
    for(int i=0;i<tar_nAttrs;i++)
    {
      AttrCatEntry attrCatEntry;
      int ret = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatEntry);
      if(ret!=SUCCESS)
        return ret;
      attr_offset[i] = attrCatEntry.offset;
      attr_types[i] = attrCatEntry.attrType;
    }


    /* Creating and opening the target relation */

    // Create a relation for target relation by calling Schema::createRel()
    int ret = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types);

    // if the createRel returns an error code, then return that value.
    if(ret != SUCCESS)
      return ret;

    // Open the newly created target relation by calling OpenRelTable::openRel()
    // and get the target relid
    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId<0 || targetRelId>=MAX_OPEN)
    {
      Schema::deleteRel(targetRel);
      return targetRelId;
    }
    // If opening fails, delete the target relation by calling Schema::deleteRel()
    // and return the error value from openRel()

    /* Inserting projected records into the target relation */

    // Take care to reset the searchIndex before calling the project function
    // using RelCacheTable::resetSearchIndex()
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[src_nAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS) {
        // the variable record will contain the next record

        Attribute proj_record[tar_nAttrs];

        //iterate through 0 to tar_attrs-1:
        //    proj_record[attr_iter] = record[attr_offset[attr_iter]]
        for(int i=0;i<tar_nAttrs;i++)
        {
          proj_record[i] = record[attr_offset[i]];
        }

        ret = BlockAccess::insert(targetRelId, proj_record);

        if (ret != SUCCESS)
        {
            // close the targetrel by calling Schema::closeRel()
          Schema::closeRel(targetRel);
            // delete targetrel by calling Schema::deleteRel()
          Schema::deleteRel(targetRel);
            // return ret;
          return ret;
        }
    }

    // Close the targetRel by calling Schema::closeRel()
    Schema::closeRel(targetRel);

    // return SUCCESS.
    return SUCCESS;
}

int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE]) {

    // get the srcRelation1's rel-id using OpenRelTable::getRelId() method
    int srcRelId1 = OpenRelTable::getRelId(srcRelation1);

    // get the srcRelation2's rel-id using OpenRelTable::getRelId() method
    int srcRelId2 = OpenRelTable::getRelId(srcRelation2);

    // if either of the two source relations is not open
    //     return E_RELNOTOPEN
    if(srcRelId1 == E_RELNOTOPEN || srcRelId2 == E_RELNOTOPEN)
      return E_RELNOTOPEN;

    AttrCatEntry attrCatEntry1, attrCatEntry2;
    // get the attribute catalog entries for the following from the attribute cache
    // (using AttrCacheTable::getAttrCatEntry())
    // - attrCatEntry1 = attribute1 of srcRelation1
    // - attrCatEntry2 = attribute2 of srcRelation2
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId1,attribute1,&attrCatEntry1);
    if(ret!=SUCCESS)
      return E_ATTRNOTEXIST;
    ret = AttrCacheTable::getAttrCatEntry(srcRelId2,attribute2,&attrCatEntry2);
    if(ret!=SUCCESS)
      return E_ATTRNOTEXIST;

    // if attribute1 is not present in srcRelation1 or attribute2 is not
    // present in srcRelation2 (getAttrCatEntry() returned E_ATTRNOTEXIST)
    //     return E_ATTRNOTEXIST.

    // if attribute1 and attribute2 are of different types return E_ATTRTYPEMISMATCH
    if(attrCatEntry1.attrType != attrCatEntry2.attrType)
      return E_ATTRTYPEMISMATCH;

    // iterate through all the attributes in both the source relations and check if
    // there are any other pair of attributes other than join attributes
    // (i.e. attribute1 and attribute2) with duplicate names in srcRelation1 and
    // srcRelation2 (use AttrCacheTable::getAttrCatEntry())
    // If yes, return E_DUPLICATEATTR
    RelCatEntry relCatEntry1,relCatEntry2;

    // get the relation catalog entries for the relations from the relation cache
    // (use RelCacheTable::getRelCatEntry() function)
    RelCacheTable::getRelCatEntry(srcRelId1,&relCatEntry1);
    RelCacheTable::getRelCatEntry(srcRelId2,&relCatEntry2);

    int numOfAttributes1 = relCatEntry1.numAttrs;/* number of attributes in srcRelation1 */
    int numOfAttributes2 = relCatEntry2.numAttrs;/* number of attributes in srcRelation2 */
    for(int i=0;i<numOfAttributes1;i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId1,i,&attrCatEntry);
        for(int j=0;j<numOfAttributes2;j++)
        {
          AttrCatEntry attrCatEntry3;
          AttrCacheTable::getAttrCatEntry(srcRelId2,j,&attrCatEntry3);
          if(i!=attrCatEntry1.offset || j!=attrCatEntry2.offset)
            if(strcmp(attrCatEntry.attrName,attrCatEntry3.attrName) == 0)
              return E_DUPLICATEATTR;
        }
    }

    // if rel2 does not have an index on attr2
    //     create it using BPlusTree:bPlusCreate()
    //     if call fails, return the appropriate error code
    //     (if your implementation is correct, the only error code that will
    //      be returned here is E_DISKFULL)
    if(attrCatEntry2.rootBlock == -1)
    {
      ret = BPlusTree::bPlusCreate(srcRelId2,attribute2);
      if(ret!=SUCCESS)
        return ret;
    }

    int numOfAttributesInTarget = numOfAttributes1 + numOfAttributes2 - 1;
    // Note: The target relation has number of attributes one less than
    // nAttrs1+nAttrs2 (Why?)

    // declare the following arrays to store the details of the target relation
    char targetRelAttrNames[numOfAttributesInTarget][ATTR_SIZE];
    int targetRelAttrTypes[numOfAttributesInTarget];

    // iterate through all the attributes in both the source relations and
    // update targetRelAttrNames[],targetRelAttrTypes[] arrays excluding attribute2
    // in srcRelation2 (use AttrCacheTable::getAttrCatEntry())
    for(int i=0;i<numOfAttributes1;i++)
    {
      AttrCatEntry attrCatEntry;
      AttrCacheTable::getAttrCatEntry(srcRelId1,i,&attrCatEntry);
      strcpy(targetRelAttrNames[i], attrCatEntry.attrName);
      targetRelAttrTypes[i] = attrCatEntry.attrType;
    }
    int flag=0;
    for(int i=0;i<numOfAttributes2;i++)
    {
      AttrCatEntry attrCatEntry;
      AttrCacheTable::getAttrCatEntry(srcRelId2,i,&attrCatEntry);
      if(i!=attrCatEntry2.offset)
      {
        strcpy(targetRelAttrNames[i+numOfAttributes1-flag], attrCatEntry.attrName);
        targetRelAttrTypes[i+numOfAttributes1-flag] = attrCatEntry.attrType;
      }
      else
        flag=1;
    }

    // create the target relation using the Schema::createRel() function

    // if createRel() returns an error, return that error
    ret = Schema::createRel(targetRelation,numOfAttributesInTarget, targetRelAttrNames,targetRelAttrTypes);
    if(ret!=SUCCESS)
      return ret;


    // Open the targetRelation using OpenRelTable::openRel()
    int relId = OpenRelTable::openRel(targetRelation);

    // if openRel() fails (No free entries left in the Open Relation Table)
    if(relId <0 || relId>=MAX_OPEN)
    {
        // delete target relation by calling Schema::deleteRel()
        Schema::deleteRel(targetRelation);
        // return the error code
        return relId;
    }

    Attribute record1[numOfAttributes1];
    Attribute record2[numOfAttributes2];
    Attribute targetRecord[numOfAttributesInTarget];
    RelCacheTable::resetSearchIndex(srcRelId1);

    // this loop is to get every record of the srcRelation1 one by one
    while (BlockAccess::project(srcRelId1, record1) == SUCCESS) {

        // reset the search index of `srcRelation2` in the relation cache
        // using RelCacheTable::resetSearchIndex()
        RelCacheTable::resetSearchIndex(srcRelId2);

        // reset the search index of `attribute2` in the attribute cache
        // using AttrCacheTable::resetSearchIndex()
        AttrCacheTable::resetSearchIndex(srcRelId2,attribute2);

        // this loop is to get every record of the srcRelation2 which satisfies
        //the following condition:
        // record1.attribute1 = record2.attribute2 (i.e. Equi-Join condition)
        while (BlockAccess::search(
            srcRelId2, record2, attribute2, record1[attrCatEntry1.offset], EQ
        ) == SUCCESS ) {

            // copy srcRelation1's and srcRelation2's attribute values(except
            // for attribute2 in rel2) from record1 and record2 to targetRecord
            for(int i=0;i<numOfAttributes1;i++)
              targetRecord[i] = record1[i];
            flag=0;
            for(int i=0;i<numOfAttributes2;i++)
            {
              if(i==attrCatEntry2.offset)
                flag=1;
              else
                targetRecord[i+numOfAttributes1-flag] = record2[i];
            }

            // insert the current record into the target relation by calling
            // BlockAccess::insert()
            int retVal = BlockAccess::insert(relId,targetRecord);

            if(retVal == E_DISKFULL) {

                // close the target relation by calling OpenRelTable::closeRel()
                OpenRelTable::closeRel(relId);
                // delete targetRelation (by calling Schema::deleteRel())
                Schema::deleteRel(targetRelation);
                return E_DISKFULL;
            }
        }
    }

    // close the target relation by calling OpenRelTable::closeRel()
    OpenRelTable::closeRel(relId);
    return SUCCESS;
}

