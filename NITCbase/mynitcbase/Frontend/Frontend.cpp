#include "Frontend.h"

#include <cstring>
#include <iostream>

int Frontend::create_table(char relname[ATTR_SIZE], int no_attrs, char attributes[][ATTR_SIZE],
                           int type_attrs[]) {
  // Schema::createRel
  return Schema::createRel(relname, no_attrs, attributes, type_attrs);
}

int Frontend::drop_table(char relname[ATTR_SIZE]) {
  // Schema::deleteRel
  return Schema::deleteRel(relname);
}

int Frontend::open_table(char relname[ATTR_SIZE]) {
  return Schema::openRel(relname);
}

int Frontend::close_table(char relname[ATTR_SIZE]) {
  return Schema::closeRel(relname);
}

int Frontend::alter_table_rename(char relname_from[ATTR_SIZE], char relname_to[ATTR_SIZE]) {
  return Schema::renameRel(relname_from, relname_to);
}

int Frontend::alter_table_rename_column(char relname[ATTR_SIZE], char attrname_from[ATTR_SIZE],
                                        char attrname_to[ATTR_SIZE]) {
  return Schema::renameAttr(relname, attrname_from, attrname_to);
}

int Frontend::create_index(char relname[ATTR_SIZE], char attrname[ATTR_SIZE]) {
  return Schema::createIndex(relname,attrname);
  // return SUCCESS;
}

int Frontend::drop_index(char relname[ATTR_SIZE], char attrname[ATTR_SIZE]) {
  return Schema::dropIndex(relname,attrname);
  // return SUCCESS;
}

int Frontend::insert_into_table_values(char relname[ATTR_SIZE], int attr_count, char attr_values[][ATTR_SIZE]) {
  // Algebra::insert
  return Algebra::insert(relname, attr_count, attr_values);
}

int Frontend::select_from_table(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE]) {
  // Algebra::project
  return Algebra::project(relname_source, relname_target);
}

int Frontend::select_attrlist_from_table(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
                                         int attr_count, char attr_list[][ATTR_SIZE]) {
  // Algebra::project
  return Algebra::project(relname_source, relname_target, attr_count, attr_list);
}

int Frontend::select_from_table_where(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
                                      char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]) {
  return Algebra::select(relname_source, relname_target, attribute, op, value);
  // return SUCCESS;
}

int Frontend::select_attrlist_from_table_where(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
                                               int attr_count, char attr_list[][ATTR_SIZE],
                                               char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]) {

      // Call select() method of the Algebra Layer with correct arguments to
    // create a temporary target relation with name ".temp" (use constant TEMP)
  char temp[ATTR_SIZE] = TEMP;
    int ret = Algebra::select(relname_source, temp, attribute, op, value);

    // TEMP will contain all the attributes of the source relation as it is the
    // result of a select operation
    if(ret != SUCCESS)
      return ret;

    // Return Error values, if not successful

    // Open the TEMP relation using OpenRelTable::openRel()
    int relid = OpenRelTable::openRel(temp);
    // if open fails, delete TEMP relation using Schema::deleteRel() and
    if(relid<0 || relid>=MAX_OPEN)
    {
      Schema::deleteRel(temp);
      return relid;
    }
    // return the error code

    // On the TEMP relation, call project() method of the Algebra Layer with
    // correct arguments to create the actual target relation. The final
    // target relation contains only those attributes mentioned in attr_list

    ret = Algebra::project(temp, relname_target, attr_count, attr_list);

    // close the TEMP relation using OpenRelTable::closeRel()
    Schema::closeRel(temp);
    // delete the TEMP relation using Schema::deleteRel()
    Schema::deleteRel(temp);
    return ret;
    // return any error codes from project() or SUCCESS otherwise
}

int Frontend::select_from_join_where(char relname_source_one[ATTR_SIZE], char relname_source_two[ATTR_SIZE],
                                     char relname_target[ATTR_SIZE],
                                     char join_attr_one[ATTR_SIZE], char join_attr_two[ATTR_SIZE]) {
  // Algebra::join
  return Algebra::join(relname_source_one, relname_source_two, relname_target, join_attr_one, join_attr_two);
}

int Frontend::select_attrlist_from_join_where(
    char relname_source_one[ATTR_SIZE], char relname_source_two[ATTR_SIZE],
    char relname_target[ATTR_SIZE], char join_attr_one[ATTR_SIZE],
    char join_attr_two[ATTR_SIZE], int attr_count, char attr_list[][ATTR_SIZE]) {

    // Call join() method of the Algebra Layer with correct arguments to
    // create a temporary target relation with name TEMP.
    char temp[ATTR_SIZE] = TEMP;
    int ret = Algebra::join(relname_source_one, relname_source_two, temp, join_attr_one, join_attr_two);

    // TEMP results from the join of the two source relation (and hence it
    // contains all attributes of the source relations except the join attribute
    // of the second source relation)
    if(ret!=SUCCESS)
      return ret;

    // Return Error values, if not successful

    // Open the TEMP relation using OpenRelTable::openRel()
    // if open fails, delete TEMP relation using Schema::deleteRel() and
    // return the error code
    int relId = OpenRelTable::openRel(temp);
    if(relId <0 || relId>=MAX_OPEN)
    {
        Schema::deleteRel(temp);
        return relId;
    }

    // Call project() method of the Algebra Layer with correct arguments to
    // create the actual target relation from the TEMP relation.
    // (The final target relation contains only those attributes mentioned in attr_list)
    ret = Algebra::project(temp, relname_target, attr_count,attr_list);

    // close the TEMP relation using OpenRelTable::closeRel()
    // delete the TEMP relation using Schema::deleteRel()
    OpenRelTable::closeRel(relId);
    Schema::deleteRel(temp);

    return ret;

    // Return Success or Error values appropriately
}

int Frontend::custom_function(int argc, char argv[][ATTR_SIZE]) {
  // argc gives the size of the argv array
  // argv stores every token delimited by space and comma

  // implement whatever you desire

  return SUCCESS;
}
