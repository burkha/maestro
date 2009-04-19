#ifndef UTIL_H_
#define UTIL_H_

#include <assert.h>
#include <iostream>

#define ASSERT_EQ(A, B)						\
  if (A != B) {							\
    std::cout << "Expected " << A << " == " << B << std::endl;	\
    assert(A == B);						\
  }

template <typename TypeA, typename TypeB>
void CastCopy(TypeA* destination, TypeB* source, size_t count) {
  TypeB* end = source + count;
  for (; source != end; ++destination, ++source) {
    *destination = static_cast<TypeA>(*source);
  }
}

template <typename Type>
class scoped_array {
 public:
  explicit scoped_array(Type* array) : array_(array) {}
  ~scoped_array() { if (array_) delete[] array_; }

  inline Type& operator[](size_t index) { return array_[index]; }
  Type* get() const { return array_; }

 private:
  Type* array_;
};

#endif  // UTIL_H_
