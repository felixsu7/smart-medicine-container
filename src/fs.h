#ifndef FILES_H
#define FILES_H

#include <cstddef>

class Filesystem {
public:
  Filesystem();

  // Loads a file from the path in the filesystsem into dest with specified
  // size. Returns -1 if the file doesn't exist. Returns -2 if the file actual
  // size is different from specified size. TODO FIXME
  static int read(const char *path, char *dest, size_t size);

  // Saves a file into the path in the filesystem from src with specified size.
  // Returns -1 if the file cannot be saved. Returns -2 if there is size
  // difference between the actual saved file and the specified size. TODO FIXME
  static int write(const char *path, const char *src, size_t size);

  // Formats the Filesystem, clearing all saved data. Returns if the operation
  // is successful or not.
  static bool format();
};

#endif
