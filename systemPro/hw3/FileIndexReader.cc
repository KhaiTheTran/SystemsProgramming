/*
 * Copyright ©2018 Justin Hsia.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Spring Quarter 2018 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <sys/types.h>  // for stat()
#include <sys/stat.h>   // for stat()
#include <unistd.h>     // for stat()

#include "./fileindexutil.h"   // for class CRC32.
#include "./FileIndexReader.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

namespace hw3 {

FileIndexReader::FileIndexReader(std::string filename,
                                 bool validate) {
  // Stash a copy of the index file's name.
  filename_ = filename;

  // Open a (FILE *) associated with filename.  Crash on error.
  file_ = fopen(filename_.c_str(), "rb");
  Verify333(file_ != nullptr);

  // Make the (FILE *) be unbuffered.  ("man setbuf")
  // MISSING:
  setbuf(file_, nullptr);  

  // Read the entire file header and convert to host format.
  // MISSING:
  HWSize_t index_size, doctable_size;
  uint32_t  magic_number, checksum;

  // check sum header field
  //Verify333(fread(&checksum, sizeof(checksum), 1, file_) == 1);

  // check magic number
  //Verify333(fread(&magic_number, sizeof(magic_number), 1, file_) == 1);
  
  // check sum header field
  //Verify333(fread(&checksum, sizeof(checksum), 1, file_) == 1);
  
// Read the entire file header and convert to host format
  IndexFileHeader hearder_ = {magic_number, checksum, doctable_size, index_size};
  //hearder_.toHostFormat();

  // Verify that the magic number is correct.  Crash if not.
  // MISSING:
  Verify333(fread(&magic_number, sizeof(magic_number), 1, file_) == 1);
  magic_number = ntohl(magic_number);
  Verify333(magic_number == 0xCAFEF00D);
  Verify333(fread(&checksum, sizeof(checksum), 1, file_) == 1);
  hearder_.toHostFormat();

  // Make sure the index file's length lines up with the header fields.
  struct stat f_stat;
  Verify333(stat(filename_.c_str(), &f_stat) == 0);
  Verify333((HWSize_t) f_stat.st_size ==
            (HWSize_t) (sizeof(IndexFileHeader) +
                        header_.doctable_size +
                        header_.index_size));

  if (validate) {
    // Re-calculate the checksum, make sure it matches that in the header.
    // Use fread() and pass the bytes you read into the crcobj.
    // Note you don't need to do any host/network order conversion,
    // since we're doing this byte-by-byte.
    CRC32 crcobj;
    uint8_t buf[512];
    HWSize_t left_to_read = header_.doctable_size + header_.index_size;
    HWSize_t i, readB;
    while (left_to_read > 0) {
      // MISSING:
      readB = fread(reinterpret_cast<void*>(buf), 1, sizeof(buf), file_);
      left_to_read -= readB;
      for(i = 0; i < readB; i++){
        crcobj.FoldByteIntoCRC(buf[i]);
      }
     }
     Verify333(crcobj.GetFinalCRC() == header_.checksum);
  }

  // Everything looks good; we're done!
}

FileIndexReader::~FileIndexReader() {
  // Close the (FILE *).
  Verify333(fclose(file_) == 0);
}

DocTableReader FileIndexReader::GetDocTableReader() {
  // The docid->name mapping starts at offset sizeof(IndexFileHeader) in
  // the index file.  Be sure to dup the (FILE *) rather than sharing
  // it across objects, just so that we don't end up with the possibility
  // of threads contending for the (FILE *) and associated with race
  // conditions.
  FILE *fdup = FileDup(file_);
  IndexFileOffset_t file_offset = sizeof(IndexFileHeader);
  return DocTableReader(fdup, file_offset);
}

IndexTableReader FileIndexReader::GetIndexTableReader() {
  // The index (word-->docid table) mapping starts at offset
  // (sizeof(IndexFileHeader) + doctable_size_) in the index file.  Be
  // sure to dup the (FILE *) rather than sharing it across objects,
  // just so that we don't end up with the possibility of threads
  // contending for the (FILE *) and associated race conditions.
  return IndexTableReader(FileDup(file_),
                          sizeof(IndexFileHeader) + header_.doctable_size);
}

}  // namespace hw3
