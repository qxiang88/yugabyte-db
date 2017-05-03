// Copyright (c) YugaByte, Inc.

#include "yb/util/bytes_formatter.h"
#include "yb/util/cast.h"
#include "yb/util/fast_varint.h"

using std::string;

namespace yb {
namespace util {

namespace {

// VarInt encoding of -2^63. This is a special case because it will cause an overflow if we try to
// negate this number and go through the positive codepath.
//
// Explanation of this particular encoding: when negated, the first two three bytes of the encoded
// representation of -2^63 become 11111111 11000000 10000000, which means it takes 10 bytes, and the
// absolute value is exactly 2^63 (with 7 zero bytes following).
const uint8_t kInt64MinValueEncoding[10] {
    0x00, 0x3f, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

struct VarIntSizeTable {
  char varint_size[256];

  VarIntSizeTable() {
    for (int i = 0; i < 256; ++i) {
      int n_high_bits = 0;
      int tmp = i;
      while (tmp & 0x80) {
        tmp <<= 1;
        n_high_bits++;
      }
      varint_size[i] = n_high_bits;
    }
  }

};

static const VarIntSizeTable kVarIntSizeTable;

inline Status NotEnoughEncodedBytes(int decoded_varint_size, int bytes_provided) {
  return STATUS_SUBSTITUTE(
      Corruption,
      "Decoded VarInt size as $0 but only $1 bytes provided",
      decoded_varint_size, bytes_provided);
}

}  // anonymous namespace

int SignedPositiveVarIntLength(int64_t v) {
  // Compute the number of bytes needed to represent this number.
  uint64_t tmp = static_cast<uint64_t>(v) << 1;
  int n = 0;
  while (tmp != 0) {
    tmp >>= 7;
    n += 1;
  }
  if (n == 0) {
    n = 1;
  }
  return n;
}

/*
 * - First bit is sign bit (0 for negative, 1 for positive)
 * - The rest is the unsigned representation of the absolute value, except for 2 differences:
 *    > The size prefix is (|v|+1)/7 instead of |v|/7 (Where |v| is the bit length of v)
 *    > For negative numbers, we have to complement everything (Note that we don't add 1, this
 *        is one's complement, not two's complement. So comp(v) = 2^|v|-1-v, not 2^|v|-v)
 *
 *    Bytes  Max Magnitude        Positives         Negatives
 *    -----  ---------            ---------         --------
 *    1      2^6-1 (63)           10[v]             01{-v}
 *    2      2^13-1 (8191)        110[v]            001{-v}
 *    3      2^20-1               1110[v]           0001{-v}
 *    4      2^27-1               11110[v]          00001{-v}
 *    5      2^34-1               111110[v]         000001{-v}
 *    6      2^41-1               1111110[v]        0000001{-v}
 *    7      2^48-1               11111110 [v]      00000001 {-v}
 *    8      2^55-1               11111111 0[v]     00000000 1{-v}
 *    9      2^62-1               11111111 10[v]    00000000 01{-v}
 *   10      2^69-1               11111111 110[v]   00000000 001{-v}
 */
void FastEncodeSignedVarInt(int64_t v, uint8_t *dest, int *size) {
  int sign = 1;
  if (v == numeric_limits<int64_t>::min()) {
    // This is a special case because we can't use the positive value codepath (negating this value
    // yields the same value in an int64_t).
    *size = sizeof(kInt64MinValueEncoding);
    memcpy(dest, kInt64MinValueEncoding, sizeof(kInt64MinValueEncoding));
    return;
  }

  if (v < 0) {
    sign = -1;
    v = -v;
  }
  const uint64_t uv = static_cast<uint64_t>(v);

  const int n = SignedPositiveVarIntLength(v);
  *size = n;

  // For n = 1 we should get 128 (10000000) as the first byte.
  //   In this case we have 6 available bits to use in the first byte.
  //
  // For n = 2, we'll get ~63 = 192 (11000000) as the first byte.
  //   In this case we have 5 available bits in the first byte, and 8 in the second byte.
  //
  // For n = 3, we'll get ~31 = 224 (11100000) as the first byte.
  //   In this case we have 4 + 8 + 8 = 20 bits to use.
  // . . .
  //
  // For n = 8, we'll get 255 (11111111). Also, the next byte's highest bit is zero.
  //   In this case we have 7 available bits in the second byte and 8 in every byte afterwards,
  //   so 55 total available bits.
  //
  // For n = 9, we'll get 255 (11111111). Also, the next byte's two highest bits are 10.
  //   In this case we have 62 more bits to use.
  //
  // For n = 10, we'll get 255 (11111111). Also, the next byte's three highest bits are 110.
  //   In this case we have 69 (5 + 8 * 8) bits to use.

  int i;
  if (n == 10) {
    dest[0] = 0xff;
    // With a 10-byte encoding we are using 8 whole bytes to represent the number, which takes 63
    // bits or fewer, so no bits from the second byte are used, and it is always 11000000 (binary).
    dest[1] = 0xc0;
    i = 2;
  } else if (n == 9) {
    dest[0] = 0xff;
    // With 9-byte encoding bits 56-61 (6 bits) of the number go into the lower 6 bits of the second
    // byte, so it becomes 10xxxxxx where xxxxxx are the said 6 bits.
    dest[1] = 0x80 | (uv >> 56);
    i = 2;
  } else {
    // In these cases
    dest[0] = ~((1 << (8 - n)) - 1) | (uv >> (8 * (n - 1)));
    i = 1;
  }
  for (; i < n; ++i) {
    dest[i] = uv >> (8 * (n - 1 - i));
  }
  if (sign == -1) {
    for (int i = 0; i < n; ++i) {
      dest[i] = ~dest[i];
    }
  }
}

std::string FastEncodeSignedVarIntToStr(int64_t v) {
  string s;
  char buf[16];
  int len = 0;
  FastEncodeSignedVarInt(v, to_uchar_ptr(buf), &len);
  DCHECK_LE(len, 10);
  s.append(buf, len);
  return s;
}

Status FastDecodeVarInt(const uint8_t* src, int src_size, int64_t* v, int* decoded_size) {
  uint8_t buf[16];
  if (src_size == 0) {
    return STATUS(Corruption, "Cannot decode a variable-length integer of zero size");
  }

  const uint8_t* const orig_src = src;

  int sign;
  int n_bytes;
  uint8_t first_byte = static_cast<uint8_t>(src[0]);
  if (first_byte & 0x80) {
    sign = 1;

    n_bytes = kVarIntSizeTable.varint_size[first_byte];
    if (src_size < n_bytes) {
      return NotEnoughEncodedBytes(n_bytes, src_size);
    }
    if (n_bytes == 1) {
      // Fast path for single-byte decoding.
      *v = first_byte & 0x3f;
      *decoded_size = 1;
      return Status::OK();
    }
  } else {
    sign = -1;
    first_byte = ~first_byte;
    n_bytes = kVarIntSizeTable.varint_size[first_byte];
    if (src_size < n_bytes) {
      return NotEnoughEncodedBytes(n_bytes, src_size);
    }
    if (n_bytes == 1) {
      // Fast path for one byte: take 6 lower bits of the inverted byte, that will give us the
      // decoded positive value, then negate it to get the result.
      *v = -(first_byte & 0x3f);
      *decoded_size = 1;
      return Status::OK();
    }

    // We know we have at least two bytes now. Copy them to the negated buffer. We'll copy the rest
    // later as we figure out the real encoded size (could be 9 or 10 bytes, but the value of n
    // is min(8, encoded_size) at this point).
    buf[0] = first_byte;
    buf[1] = ~orig_src[1];
    src = buf;
  }

  uint64_t result = 0;
  int i = 0;
  if (n_bytes == 8) {
    if (src[1] & 0x80) {
      if (src[1] & 0x40) {
        n_bytes = 10;
        result = src[1] & 0x1f;
        i = 2;
      } else {
        n_bytes = 9;
        result = src[1] & 0x3f;
        i = 2;
      }
      // For encoded size of 9 and 10 we have to do the length check again, because the previously
      // we only checked that we have at least 8 bytes.
      if (src_size < n_bytes) {
        return NotEnoughEncodedBytes(n_bytes, src_size);
      }
    } else {
      i = 1;
    }
  } else {
    result = src[0] & ((1 << (7 - n_bytes)) - 1);
    i = 1;
  }

  if (sign == -1) {
    // In this case src is already pointing to the local buffer. Copy negated bytes into our local
    // buffer so we can decode them using the same logic as for positive VarInts.
    memcpy(buf, orig_src, n_bytes);
    for (int i = 0; i < n_bytes; ++i) {
      buf[i] = ~buf[i];
    }
  }

  for (; i < n_bytes; ++i) {
    result = (result << 8) | static_cast<uint8_t>(src[i]);
  }

  int64_t signed_result = static_cast<int64_t>(result);
  if (sign < 0 && signed_result != std::numeric_limits<int64_t>::min()) {
    signed_result = -signed_result;
  }

  *v = signed_result;
  *decoded_size = n_bytes;
  return Status::OK();
}

Status FastDecodeVarInt(std::string encoded, int64_t* v, int* decoded_size) {
  return FastDecodeVarInt(to_uchar_ptr(encoded.c_str()), encoded.size(), v, decoded_size);
}

Status FastDecodeDescendingVarInt(yb::Slice *slice, int64_t *dest) {
  int decoded_size = 0;
  RETURN_NOT_OK(FastDecodeVarInt(slice->data(), slice->size(), dest, &decoded_size));
  *dest = -*dest;
  slice->remove_prefix(decoded_size);
  return Status::OK();
}

}  // namespace util
}  // namespace yb
