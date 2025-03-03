#include "StaticBuffer.h"
// the declarations for this class can be found at "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {
  for(int i=0;i<4;i++)
  {
    Disk::readBlock(&blockAllocMap[i*BLOCK_SIZE],i);
  }
  // initialise all blocks as free
  for (int bufferIndex = 0; bufferIndex<BUFFER_CAPACITY; bufferIndex++) {
    metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].dirty = false;
    metainfo[bufferIndex].timeStamp = -1;
    metainfo[bufferIndex].blockNum = -1;
  }
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer() {

  for(int i=0;i<4;i++)
  {
    Disk::writeBlock(&blockAllocMap[i*BLOCK_SIZE],i);
  }

  for(int bufferIndex = 0; bufferIndex<BUFFER_CAPACITY; bufferIndex++)
  {
    if(metainfo[bufferIndex].free == false && metainfo[bufferIndex].dirty == true)
    {
      Disk::writeBlock(blocks[bufferIndex],metainfo[bufferIndex].blockNum);
    }
  }

}

int StaticBuffer::getFreeBuffer(int blockNum) {
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  for(int i=0;i<BUFFER_CAPACITY;i++)
  {
    if(metainfo[i].free==false)
    {
        metainfo[i].timeStamp+=1;
    }
  }
  int bufferNum;
  int i;
  for(i=0;i<BUFFER_CAPACITY;i++)
  {
    if(metainfo[i].free==true)
    {
        bufferNum = i;
        break;
    }
  }
  if(i == BUFFER_CAPACITY)
  {
    int max=0;
    for(int j=0;j < BUFFER_CAPACITY;j++)
    {
      if(metainfo[j].timeStamp > metainfo[max].timeStamp)
      {
        max = j;
      }
    }
    if(metainfo[max].dirty == true)
    {
      Disk::writeBlock(blocks[max],metainfo[max].blockNum);
    }
    bufferNum = max;
  }

  metainfo[bufferNum].free = false;
  metainfo[bufferNum].dirty = false;
  metainfo[bufferNum].blockNum = blockNum;
  metainfo[bufferNum].timeStamp = 0;

  return bufferNum;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS)
  // and return E_OUTOFBOUND if not valid.
    if (blockNum < 0 || blockNum >= DISK_BLOCKS)
    {
      return E_OUTOFBOUND;
    }

  // find and return the bufferIndex which corresponds to blockNum (check metainfo)
  for (int bufferIndex = 0; bufferIndex<BUFFER_CAPACITY; bufferIndex++)
  {
    if(metainfo[bufferIndex].blockNum == blockNum)
        return bufferIndex;
  }


  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum){
    // find the buffer index corresponding to the block using getBufferNum().
    int bufferIndex = getBufferNum(blockNum);
    if(bufferIndex == E_BLOCKNOTINBUFFER)
      return bufferIndex;
    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    //     return E_BLOCKNOTINBUFFER
    if(bufferIndex == E_OUTOFBOUND)
      return bufferIndex;
    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
    //     return E_OUTOFBOUND
    metainfo[bufferIndex].dirty = true;
    return SUCCESS;
    // else
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo

    // return SUCCESS
}

int StaticBuffer::getStaticBlockType(int blockNum){
    // Check if blockNum is valid (non zero and less than number of disk blocks)
    // and return E_OUTOFBOUND if not valid.
    if (blockNum < 0 || blockNum >= DISK_BLOCKS)
    {
      return E_OUTOFBOUND;
    }

    // Access the entry in block allocation map corresponding to the blockNum argument
    int type = (int) blockAllocMap[blockNum];
    // and return the block type after type casting to integer.
    return type;
}
