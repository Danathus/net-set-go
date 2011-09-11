#ifndef SERIALIZATION_H
#define SERIALIZATION_H

////////////////////////////////////////////////////////////////////////////////

class Serializable
{
public:
   /*
    * Writes data to a byte buffer.
    * @param buffer Pointer to the buffer to be written to.
    * @param numBytes The number of bytes available in the buffer to be written to.
    * @return The number of bytes written; -1 indicates failure
    */
   virtual size_t Serialize(unsigned char* buffer, size_t numBytes) const = 0;
   virtual size_t Deserialize(const unsigned char* buffer, size_t numBytes) = 0;

   virtual size_t GetSize() const = 0;

protected:
private:
};

////////////////////////////////////////////////////////////////////////////////

// helper functions for reading and writing integer values to packets

inline size_t WriteByte(unsigned char* data, unsigned char value)
{
   *data = value;

   return sizeof(char);
}

inline size_t ReadByte(const unsigned char* data, unsigned char& value)
{
   value = *data;

   return sizeof(char);
}

inline size_t WriteShort(unsigned char* data, unsigned short value)
{
   data[0] = (unsigned char)((value >> 8) & 0xFF);
   data[1] = (unsigned char)(value & 0xFF);

   return sizeof(short);
}

inline size_t ReadShort(const unsigned char* data, unsigned short& value)
{
   value = (((unsigned int)data[0] << 8)  | ((unsigned int)data[1]));

   return sizeof(short);
}

inline size_t WriteInteger(unsigned char* data, unsigned int value)
{
   data[0] = (unsigned char) (value >> 24);
   data[1] = (unsigned char)((value >> 16) & 0xFF);
   data[2] = (unsigned char)((value >> 8)  & 0xFF);
   data[3] = (unsigned char) (value        & 0xFF);

   return sizeof(int);
}

inline size_t ReadInteger(const unsigned char* data, unsigned int& value)
{
   value = (((unsigned int)data[0] << 24) | ((unsigned int)data[1] << 16) |
            ((unsigned int)data[2] << 8)  | ((unsigned int)data[3]));

   return sizeof(int);
}

////////////////////////////////////////////////////////////////////////////////

#endif // SERIALIZATION_H
