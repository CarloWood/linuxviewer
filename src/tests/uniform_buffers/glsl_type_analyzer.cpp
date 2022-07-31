// Compile:
//
//  Download the header pstream.h from http://pstreams.sourceforge.net/
//
//  Then run:
//
//  g++ -std=c++20 -Wno-deprecated-enum-enum-conversion -I pstreams-1.0.3 glsl_type_analyzer.cpp
//
#include "pstream.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <array>
#include <string>
#include <cassert>
#include <string_view>
#include <fstream>
#include <regex>
#include <algorithm>
#include <cctype>

// Basic types.

// Builtin-types
constexpr int Float = 0;
constexpr int Double = 1;
constexpr int Bool = 2;
constexpr int Int = 3;
constexpr int Uint = 4;
constexpr int number_of_builtin_types = 5;

constexpr std::array<char const*, number_of_builtin_types> builtin_types = {
  "float",
  "double",
  "bool",
  "int",
  "uint"
};

std::array<std::string, number_of_builtin_types> const type_prefix = {
  "",   // None
  "d",
  "b",
  "i",
  "u"
};

std::array<int, number_of_builtin_types> const base_size = {
  4,
  8,
  4,
  4,
  4
};

enum Kind {
  Scalar,
  Vector,
  Matrix
};

std::string make_type_name(int scalar_type, int rows, int cols)
{
  // Check out of range.
  assert(0 <= scalar_type && scalar_type < number_of_builtin_types);
  assert(1 <= rows && rows <= 4);
  assert(1 <= cols && cols <= 4);
  // Row-vectors are not used in this rows/cols encoding.
  assert(!(rows == 1 && cols > 1));

  Kind kind = (rows == 1) ? Scalar : (cols == 1) ? Vector : Matrix;
  // There are only matrices of float and double.
  assert(kind != Matrix || (scalar_type == Float || scalar_type == Double));

  std::string type_name = type_prefix[scalar_type];
  switch (kind)
  {
    case Scalar:
      type_name = builtin_types[scalar_type];
      break;
    case Vector:
      type_name += "vec" + std::to_string(rows);
      break;
    case Matrix:
      type_name += "mat" + std::to_string(cols) + "x" + std::to_string(rows);
      break;
  }
  return type_name;
}

static constexpr std::string_view test_header = R"glsl(
#version 450
#extension GL_EXT_scalar_block_layout : require

struct TestStruct {
)glsl";

static constexpr std::string_view test_footer = R"glsl(
};

layout(std140, set = 0, binding = 0) uniform Uniform140 {
    TestStruct u_as;
} ubo140;                                                                           

layout(std430, set = 1, binding = 0) uniform Uniform430 {
    TestStruct u_as;
} ubo430;                                                                           

layout(scalar, set = 1, binding = 0) uniform Uniform000 {
    TestStruct u_as;
} ubo000;                                                                           

void main()
{
}
)glsl";

static constexpr std::string_view test_array_footer = R"glsl(
};

layout(std140, set = 0, binding = 0) uniform Uniform140 {
    TestStruct u_as[10];
} ubo140;                                                                           

layout(std430, set = 1, binding = 0) uniform Uniform430 {
    TestStruct u_as[10];
} ubo430;                                                                           

layout(scalar, set = 1, binding = 0) uniform Uniform000 {
    TestStruct u_as[10];
} ubo000;                                                                           

void main()
{
}
)glsl";

// OpMemberDecorate %TestStruct_0 1 Offset 4
std::regex const Offset_regex{R"~(\s*OpMemberDecorate %([\w]+) 1 Offset ([\d]+))~"};
// OpDecorate %_arr_TestStruct_0_uint_10 ArrayStride 16
std::regex const ArrayStride_regex{R"~(\s*OpDecorate %([\w]+) ArrayStride ([\d]+))~"};
// %Uniform140 = OpTypeStruct %TestStruct_0
std::regex const Uniform_OpTypeStruct_regex{R"~(\s*%Uniform([\d]+) = OpTypeStruct %([\w]+))~"};
// %TestStruct_1 = OpTypeStruct %_arr_uint_uint_10_1 %uint
std::regex const OpTypeStruct_regexp{R"~(\s*%([\w]+) = OpTypeStruct %([\w]+) %uint)~"};

enum What
{
  eAlignment,
  eSize,
  eArrayStride
};

std::string what_str(int what)
{
  switch (what)
  {
    case eAlignment:
      return "eAlignment";
    case eSize:
      return "eSize";
    case eArrayStride:
      return "eArrayStride";
  }
  return "UNKNOWN";
}

enum Standard
{
  std_scalar = 0,
  std_std140 = 1,
  std_std430 = 2,
  number_of_standards = 3
};

std::string standard_str(int st)
{
  switch (st)
  {
    case std_scalar:
      return "std_scalar";
    case std_std140:
      return "std140";
    case std_std430:
      return "std430";
  }
  return "UNKNOWN";
}

std::array<int, number_of_standards> determine(std::string type_name, What what, std::array<int, number_of_standards>& measured_array_stride)
{
#ifdef DEBUG
  std::cout << "Entering determine(" << type_name << ", " << what_str(what) << ", ...)" << std::endl;
#endif

  using namespace redi;
  pstream compiler("glslc -S -fshader-stage=vertex - -o -", pstreams::pstdin|pstreams::pstdout);

  compiler << (test_header.data() + 1);

  if (what == eAlignment)
  {
    compiler << "  bool b_wedge_0;\n";
    compiler << "  " << type_name << " v_" << type_name << "_1;";
  }
  else if (what == eSize)
  {
    compiler << "  " << type_name << " v_" << type_name << "_0;";
    compiler << "  bool b_lid_1;\n";
  }
  else // what == eArrayStride
  {
    compiler << "  " << type_name << " v_" << type_name << "_0[10];";
    compiler << "  bool b_lid_1;\n";
  }

//  if (what == array_stride)
//    compiler << test_array_footer;
//  else
    compiler << test_footer;
  compiler << peof;

  // The code below determines the offset of the second member.

  enum State
  {
    look_for_Offset,
    look_for_ArrayStride,
    look_for_OpTypeStruct
  };

  std::array<int, number_of_standards> measured_second_offset{};

  State state = (what == eArrayStride) ? look_for_ArrayStride : look_for_Offset;
  std::smatch m;
  std::map<std::string, int> name_to_offset;
  std::map<std::string, int> name_to_stride;
  std::ofstream out_file("test.spvasm");
  std::string line;
  while (std::getline(compiler, line))
  {
    out_file << line << '\n';
    switch (state)
    {
      case look_for_ArrayStride:
        if (std::regex_match(line, m, ArrayStride_regex))
        {
          std::string struct_name = m[1];
          int stride = std::atoi(m[2].str().c_str());
#ifdef DEBUG
          std::cout << "Matched: " << line << std::endl;
          std::cout << "Setting name_to_stride[" << struct_name << "] = " << stride << std::endl;
#endif
          name_to_stride[struct_name] = stride;
        }
        [[fallthrough]];
      case look_for_Offset:
        if (std::regex_match(line, m, Offset_regex))
        {
          std::string struct_name = m[1];
          int offset = std::atoi(m[2].str().c_str());
#ifdef DEBUG
          std::cout << "Matched: " << line << std::endl;
          std::cout << "Setting name_to_offset[" << struct_name << "] = " << offset << std::endl;
#endif
          name_to_offset[struct_name] = offset;
          if (name_to_offset.size() == number_of_standards)
            state = look_for_OpTypeStruct;
        }
        break;
      case look_for_OpTypeStruct:
        if (std::regex_match(line, m, Uniform_OpTypeStruct_regex))
        {
          assert(m[1] == "140" || m[1] == "430" || m[1] == "000");
          Standard standard = (m[1] == "140") ? std_std140 : (m[1] == "430") ? std_std430 : std_scalar;
          std::string struct_name = m[2];
#ifdef DEBUG
          if (name_to_offset.contains(struct_name) || name_to_stride.contains(struct_name))
            std::cout << "Matched: " << line << std::endl;
#endif
          if (name_to_offset.contains(struct_name))
          {
            int offset = name_to_offset[struct_name];
#ifdef DEBUG
            std::cout << "Setting measured_second_offset[" << standard << "] = name_to_offset[" << struct_name << "] = " << offset << std::endl;
#endif
            measured_second_offset[standard] = offset;
          }
          if (name_to_stride.contains(struct_name))
          {
            int stride = name_to_stride[struct_name];
#ifdef DEBUG
            std::cout << "Setting array_stride[" << standard << "] = name_to_stride[" << struct_name << "] = " << stride << std::endl;
#endif
            measured_array_stride[standard] = stride;
          }
        }
        else if (std::regex_match(line, m, OpTypeStruct_regexp))
        {
#ifdef DEBUG
          if (name_to_offset.contains(m[2]) || name_to_stride.contains(m[2]))
            std::cout << "Matched: " << line << std::endl;
#endif
          // Propagate spir-v names.
          if (name_to_offset.contains(m[2]))
          {
#ifdef DEBUG
            std::cout << "Setting name_to_offset[" << m[1] << "] = name_to_offset[" << m[2] << "] = " << name_to_offset[m[2]] << std::endl;
#endif
            name_to_offset[m[1]] = name_to_offset[m[2]];
          }
          if (name_to_stride.contains(m[2]))
          {
#ifdef DEBUG
            std::cout << "Setting name_to_stride[" << m[1] << "] = name_to_stride[" << m[2] << "] = " << name_to_stride[m[2]] << std::endl;
#endif
            name_to_stride[m[1]] = name_to_stride[m[2]];
          }
        }
        break;
    }
  }
  out_file.close();

  std::cout << type_name << ":\t ";
  bool offsets_are_the_same =
    measured_second_offset[0] == measured_second_offset[1] && measured_second_offset[0] == measured_second_offset[2] && measured_second_offset[1] == measured_second_offset[2];
  char const* prefix = "";
  for (int st = 0; st < number_of_standards; ++st)
  {
    assert(measured_second_offset[st]);
    std::cout << prefix;
    prefix = "; ";
    if (!offsets_are_the_same)
      std::cout << standard_str(st) << ": ";
    if (what == eAlignment)
      std::cout << "alignment: " << measured_second_offset[st];
    else if (what == eSize)
      std::cout << "size: " << measured_second_offset[st];
    else if (what == eArrayStride)
    {
      assert(measured_array_stride[st]);
      std::cout << "next member offset: " << measured_second_offset[st] << "; array_stride: " << measured_array_stride[st];
    }

    if (offsets_are_the_same)
      break;
  }
  std::cout << std::endl;

  return measured_second_offset;
}

struct Info
{
  int scalar_type;        // 0: bool, 1: int, 2: uint, 3: float, 4: double. When this value is 0, 1 or 2 then cols is 1.
  int rows;             // If rows is 1, then cols is 1 and this is a scalar.
  int cols;             // If cols is 1 and rows > 1, then this is a vector.
  int standard;         // 0: std_std140, 1: std_std430, 2: std_scalar.
  int alignment;        // The alignment of this type when not used in an array.
  int size;             // The (padded) size of this type when not used in an array.
  int array_stride;     // The alignment and size of array elements of this type.
  int next_member;      // The minimum base offset of a member (ie, an uint) that follows an array of this type, relative to the base offset of that array.
};

std::ostream& operator<<(std::ostream& os, Info const& info)
{
  std::string type_name = make_type_name(info.scalar_type, info.rows, info.cols);
  os << "{scalar_type:" << builtin_types[info.scalar_type] << "; type: " << type_name <<
    "; standard: " << standard_str(info.standard) << "; alignment = " << info.alignment << "; size = " << info.size <<
    "; array_stride = " << info.array_stride << "; next_member = " << info.next_member << "}";
  return os;
}

std::map<int, std::map<int, std::map<int, std::map<int, Info>>>> database;

void store(int scalar_type, int rows, int cols,
    std::array<int, number_of_standards> const& alignments,
    std::array<int, number_of_standards> const& sizes,
    std::array<int, number_of_standards> const& strides,
    std::array<int, number_of_standards> const& next_members)
{
  for (int st = 0; st < number_of_standards; ++st)
  {
    database[scalar_type][rows][cols][st] = Info{
      .scalar_type = scalar_type, .rows = rows, .cols = cols, .standard = st, .alignment = alignments[st], .size = sizes[st], .array_stride = strides[st], .next_member = next_members[st]
    };
  }
}

void check_rules()
{
  // Rule 1. If the member is a scalar consuming N basic machine units, the base alignment is N.
  for (int scalar_type = 0; scalar_type < number_of_builtin_types; ++scalar_type)
  {
    // Scalar.
    int rows = 1;
    int cols = 1;
    std::string type_name = make_type_name(scalar_type, rows, cols);
    for (int st = 0; st < number_of_standards; ++st)
    {
      Info const& info{database[scalar_type][rows][cols][st]};
      assert(info.size == base_size[scalar_type]);
      assert(info.alignment == info.size);
    }
  }
  // Rule 2. If the member is a two- or four-component vector with components consuming N basic machine units, the base alignment is 2N or 4N, respectively.
  for (int scalar_type = 0; scalar_type < number_of_builtin_types; ++scalar_type)
  {
    // Two- or four-component vector.
    for (int rows = 2; rows <= 4; rows += 2)
    {
      int cols = 1;
      std::string type_name = make_type_name(scalar_type, rows, cols);
      for (int st = 0; st < number_of_standards; ++st)
      {
        if (st == std_scalar)
          continue;             // This rule doesn't hold for Scalar.
        Info const& info{database[scalar_type][rows][cols][st]};
        assert(info.alignment == rows * base_size[scalar_type]);
      }
    }
  }
  // Rule 3. If the member is a three-component vector with components consuming N basic machine units, the base alignment is 4N.
  for (int scalar_type = 0; scalar_type < number_of_builtin_types; ++scalar_type)
  {
    // Three component vector.
    int rows = 3;
    int cols = 1;
    std::string type_name = make_type_name(scalar_type, rows, cols);
    for (int st = 0; st < number_of_standards; ++st)
    {
      if (st == std_scalar)
        continue;             // This rule doesn't hold for Scalar.
      Info const& info{database[scalar_type][rows][cols][st]};
      assert(info.alignment == 4 * base_size[scalar_type]);
    }
  }
  // Rule 4. If the member is an array of scalars or vectors, the base alignment and array
  // stride are set to match the base alignment of a single array element, according
  // to rules (1), (2), and (3), and rounded up to the base alignment of a vec4.
  // The array may have padding at the end; the base offset of the member following
  // the array is rounded up to the next multiple of the base alignment.
  for (int scalar_type = 0; scalar_type < number_of_builtin_types; ++scalar_type)
  {
    // Scalar or vector.
    for (int rows = 1; rows <= 4; ++rows)
    {
      int cols = 1;
      std::string type_name = make_type_name(scalar_type, rows, cols);
      for (int st = 0; st < number_of_standards; ++st)
      {
        if (st == std_scalar)
          continue;             // This rule doesn't hold for Scalar.
        int const base_alignment_of_vec4    = database[    Float][   4][1][st].alignment;
        int const base_alignment_of_element = database[scalar_type][rows][1][st].alignment;
        // When using the std430 storage layout, shader storage blocks will be laid out in buffer storage
        // identically to uniform and shader storage blocks using the std140 layout, except that the base
        // alignment and stride of arrays of scalars and vectors in rule 4 and of structures in rule 9
        // are not rounded up a multiple of the base alignment of a vec4.
        int const expected_array_stride = (st == std_std140) ? std::max(base_alignment_of_element, base_alignment_of_vec4) : base_alignment_of_element;
        int const array_size = 10;
        Info const& info{database[scalar_type][rows][cols][st]};
        assert(info.array_stride == expected_array_stride && info.next_member == array_size * expected_array_stride);
      }
    }
  }
  // Rule 5. If the member is a column-major matrix with C columns and R rows, the
  // matrix is stored identically to an array of C column vectors with R components
  // each, according to rule (4).
  //
  // Matrices only may contain floats or doubles as base type.
  for (int scalar_type = Float; scalar_type <= Double; ++scalar_type)
  {
    // Matrix. All our matrices are the default column-major.
    for (int rows = 2; rows <= 4; ++rows)
    {
      for (int cols = 2; cols <= 4; ++cols)
      {
        std::string type_name = make_type_name(scalar_type, rows, cols);
        for (int st = 0; st < number_of_standards; ++st)
        {
          if (st == std_scalar)
            continue;             // This rule doesn't hold for Scalar.
          int const array_of_vectors_alignment = database[scalar_type][rows][1][st].array_stride;
          int const array_of_vectors_size = cols * array_of_vectors_alignment;  // The above array_stride.
          Info const& info{database[scalar_type][rows][cols][st]};
          assert(info.alignment == array_of_vectors_alignment && info.size == array_of_vectors_size);
        }
      }
    }
  }
  // Rule 6. If the member is an array of S column-major matrices with C columns and R rows,
  // the array of matrices is stored identically to an array of SxC vectors with R components,
  // according to rule (4), where each matrix column becomes a vector, ordered by the original
  // array order, and within that by column number.
  // (See https://github.com/KhronosGroup/GLSL/issues/92#issuecomment-542746491)
  //
  // Matrices only may contain floats or doubles as base type.
  for (int scalar_type = Float; scalar_type <= Double; ++scalar_type)
  {
    // Matrix. All our matrices are the default column-major.
    for (int rows = 2; rows <= 4; ++rows)
    {
      for (int cols = 2; cols <= 4; ++cols)
      {
        std::string type_name = make_type_name(scalar_type, rows, cols);
        for (int st = 0; st < number_of_standards; ++st)
        {
          Info const& vector_info{database[scalar_type][rows][1][st]};
          Info const& info{database[scalar_type][rows][cols][st]};
          assert(info.array_stride == cols * vector_info.array_stride && info.next_member == cols * vector_info.next_member);
        }
      }
    }
  }
  // Rule 9. If the member is a structure, the base alignment of the structure is N, where
  // N is the largest base alignment value of any of its members, and rounded
  // up to the base alignment of a vec4. The individual members of this substructure are
  // then assigned offsets by applying this set of rules recursively,
  // where the base offset of the first member of the sub-structure is equal to the
  // aligned offset of the structure. The structure may have padding at the end;
  // the base offset of the member following the sub-structure is rounded up to
  // the next multiple of the base alignment of the structure.
}

std::string enum_name(int scalar_type)
{
  std::string name = "TI::e";
  name += std::toupper(builtin_types[scalar_type][0]);
  name += &builtin_types[scalar_type][1];
  return name;
}

namespace glsl {

enum Standard {
  scalar = std_scalar,
  std140 = std_std140,
  std430 = std_std430
};

constexpr int number_of_glsl_types = number_of_builtin_types;
enum TypeIndex {
  eDouble = Double
};

#define ASSERT(x) assert(x)

} // namespace glsl

// Return the GLSL alignment of a scalar, vector or matrix type under the given standard.
uint32_t alignment(glsl::Standard standard, glsl::TypeIndex scalar_type, int rows, int cols)
{
  using namespace glsl;
  Kind const kind = (rows == 1) ? Scalar : (cols == 1) ? Vector : Matrix;

  // This function should not be called for vertex attribute types.
  ASSERT(scalar_type < number_of_glsl_types);

  // All scalar types have a minimum size of 4.
  uint32_t const scalar_size = (scalar_type == eDouble) ? 8 : 4;

  // The alignment is equal to the size of the (underlaying) scalar type if the standard
  // is `scalar`, and also when the type just is a scalar.
  if (kind == Scalar || standard == scalar)
    return scalar_size;

  // In the case of a vector that is multiplied with 2 or 4 (a vector with 3 rows takes the
  // same space as a vector with 4 rows).
  uint32_t const vector_alignment = scalar_size * ((rows == 2) ? 2 : 4);

  if (kind == Vector)
    return vector_alignment;

  // For matrices round that up to the alignment of a vec4 if the standard is `std140`.
  return (standard == std140) ? std::max(vector_alignment, 16U) : vector_alignment;
}

// Return the GLSL size of a scalar, vector or matrix type under the given standard.
uint32_t size(glsl::Standard standard, glsl::TypeIndex scalar_type, int rows, int cols)
{
  using namespace glsl;
  Kind const kind = (rows == 1) ? Scalar : (cols == 1) ? Vector : Matrix;

  // Non-matrices have the same size as in the standard 'std_scalar' case.
  if (kind != Matrix || standard == scalar)
    return alignment(standard, scalar_type, 1, 1) * rows * cols;

  // Matrices are layed out as arrays of `cols` column-vectors.
  // The alignment of the matrix is equal to the alignment of one such column-vector, also known as the matrix-stride.
  uint32_t const matrix_stride = alignment(standard, scalar_type, rows, cols);

  // The size of the matrix is simple the size of one of its column-vectors times the number of columns.
  return cols * matrix_stride;
}

// Return the GLSL array_stride of a scalar, vector or matrix type under the given standard.
uint32_t array_stride(glsl::Standard standard, glsl::TypeIndex scalar_type, int rows, int cols)
{
  // The array stride is equal to the largest of alignment and size.
  uint32_t array_stride = std::max(alignment(standard, scalar_type, rows, cols), size(standard, scalar_type, rows, cols));

  // In the case of std140 that must be rounded up to 16.
  return (standard == glsl::std140) ? std::max(array_stride, 16U) : array_stride;
}

int main()
{
  // Run over all basic types.
  for (int scalar_type = 0; scalar_type < number_of_builtin_types; ++scalar_type)
    for (int rows = 1; rows <= 4; ++rows)
      for (int cols = 1; cols <= ((rows == 1 || scalar_type > Double) ? 1 : 4); ++cols)
      {
        std::string type_name = make_type_name(scalar_type, rows, cols);
        std::array<int, number_of_standards> strides;
        auto alignments = determine(type_name, eAlignment, strides);
        auto sizes = determine(type_name, eSize, strides);
        auto next_members = determine(type_name, eArrayStride, strides);
        store(scalar_type, rows, cols, alignments, sizes, strides, next_members);
      }
  check_rules();
  for (int sti = 0; sti < number_of_standards; ++sti)
  {
    glsl::Standard st = static_cast<glsl::Standard>(sti);
    std::cout << '\n' << standard_str(st) << ":\n\n";
    for (int sti = 0; sti < number_of_builtin_types; ++sti)
    {
      glsl::TypeIndex scalar_type = static_cast<glsl::TypeIndex>(sti);
      for (int rows = 1; rows <= 4; ++rows)
        for (int cols = 1; cols <= ((rows == 1 || scalar_type > Double) ? 1 : 4); ++cols)
        {
          std::string type_name = make_type_name(scalar_type, rows, cols);
          Info& info = database[scalar_type][rows][cols][st];
          std::cout << std::setw(8) << type_name << ": encode(" << rows << ", " << cols << ", " << std::setw(11) << enum_name(scalar_type) <<
            ", " << std::setw(3) << info.alignment << ", " << std::setw(3) << info.size << ", " << std::setw(3) << info.array_stride << ")\n";
          // Test the three functions.
          assert(info.alignment == alignment(st, scalar_type, rows, cols));
          assert(info.size == size(st, scalar_type, rows, cols));
          assert(info.array_stride == array_stride(st, scalar_type, rows, cols));
        }
    }
  }
}
