#include <iostream>
#include <iomanip>
#include <ios>
#include <vector>
#include <tuple>
#include <type_traits>

/**
 * A class for "pretty printing" a table of data.
 *
 * Requries C++11 (and nothing more)
 *
 * It's templated on the types that will be in each column
 * (all values in a column must have the same type)
 *
 * For instance, to use it with data that looks like:  "Fred", 193.4, 35, "Sam"
 * with header names: "Name", "Weight", "Age", "Brother"
 *
 * You would invoke the table like so:
 * VariadicTable<std::string, double, int, std::string> vt({"Name", "Weight", "Age", "Brother"});
 *
 * Then add the data to the table:
 * vt.addRow({"Fred", 193.4, 35, "Sam"});
 *
 * And finally print it:
 * vt.print();
 */
template <class... Ts>
class VariadicTable
{
public:
  /// The type stored for each row
  typedef std::tuple<Ts...> DataTuple;

  /**
   * Construct the table with headers
   *
   * @param headers The names of the columns
   * @param static_column_size The size of columns that can't be found automatically
   */
  VariadicTable(std::vector<std::string> headers, unsigned int static_column_size = 0)
    : _headers(headers), _num_columns(std::tuple_size<DataTuple>::value), _static_column_size(static_column_size)
  {
    if (headers.size() != _num_columns)
    {
      std::cout << "Number of headers must match number of columns!" << std::endl;
      std::abort();
    }
  }

  /**
   * Add a row of data
   *
   * Easiest to use like:
   * table.addRow({data1, data2, data3});
   *
   * @param data A Tuple of data to add
   */
  void addRow(std::tuple<Ts...> data) { _data.push_back(data); }

  /**
   * Pretty print the table of data
   */
  template <typename StreamType>
  void print(StreamType & stream)
  {
    size_columns();

    // Start computing the total width
    // First - we will have _num_columns + 1 "|" characters
    unsigned int total_width = _num_columns + 1;

    // Now add in the size of each colum
    for (auto & col_size : _column_sizes)
      total_width += col_size;

    // Print out the top line
    stream << std::string(total_width, '-') << "\n";

    // Print out the headers
    stream << "|";
    for (unsigned int i = 0; i < _num_columns; i++)
    {
      // Must find the center of the column
      auto half = _column_sizes[i] / 2;
      half -= _headers[i].size() / 2;

      stream << std::setw(_column_sizes[i]) << std::left << std::string(half, ' ') + _headers[i] << "|";
    }

    stream << "\n";

    // Print out the line below the header
    stream << std::string(total_width, '-') << "\n";

    // Now print the rows of the table
    for (auto & row : _data)
    {
      stream << "|";
      print_each(row, stream);
      stream << "\n";
    }

    // Print out the line below the header
    stream << std::string(total_width, '-') << "\n";
  }

protected:
  // Just some handy typedefs for the following two functions
  typedef decltype(&std::right) right_type;
  typedef decltype(&std::left) left_type;

  // Attempts to figure out the correct justification for the data
  // If it's a floating point value
  template <typename T,
            typename = typename std::enable_if<
                std::is_arithmetic<typename std::remove_reference<T>::type>::value>::type>
  static right_type justify(int /*firstchoice*/)
  {
    return std::right;
  }

  // Otherwise
  template <typename T>
  static left_type justify(long /*secondchoice*/)
  {
    return std::left;
  }

  /**
   * These three functions print out each item in a Tuple into the table
   *
   * Original Idea From From https://stackoverflow.com/a/26908596
   *
   * BTW: This would all be a lot easier with generic lambdas
   * there would only need to be one of this sequence and then
   * you could pass in a generic lambda.  Unfortunately, that's C++14
   */

  /**
   *  This ends the recursion
   */
  template <typename TupleType, typename StreamType>
  void print_each(TupleType &&,
                  StreamType & stream,
                  std::integral_constant<
                      size_t,
                      std::tuple_size<typename std::remove_reference<TupleType>::type>::value>)
  {
  }

  /**
   * This gets called on each item
   */
  template <std::size_t I,
            typename TupleType,
            typename StreamType,
            typename = typename std::enable_if<
                I != std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type>
  void print_each(TupleType && t, StreamType & stream, std::integral_constant<size_t, I>)
  {
    auto & val = std::get<I>(t);

    stream << std::setw(_column_sizes[I]) << justify<decltype(val)>(0) << val << "|";

    // Recursive call to print the next item
    print_each(std::forward<TupleType>(t), stream, std::integral_constant<size_t, I + 1>());
  }

  /**
   * his is what gets called first
   */
  template <typename TupleType, typename StreamType>
  void print_each(TupleType && t, StreamType & stream)
  {
    print_each(std::forward<TupleType>(t), stream, std::integral_constant<size_t, 0>());
  }

  /**
   * Try to find the size the column will take up
   *
   * If the datatype has a size() member... let's call it
   */
  template <class T, class size_type = decltype(((T *)nullptr)->size())>
  size_t sizeOfData(const T & data)
  {
    return data.size();
  }

  /**
   * If it doesn't... let's just use a statically set size
   */
  size_t sizeOfData(...) { return _static_column_size; }

  /**
   * These three functions iterate over the Tuple, find the printed size of each element and set it
   * in a vector
   */

  /**
   * End the recursion
   */
  template <typename TupleType>
  void size_each(TupleType &&,
                 std::vector<unsigned int> & sizes,
                 std::integral_constant<
                     size_t,
                     std::tuple_size<typename std::remove_reference<TupleType>::type>::value>)
  {
  }

  /**
   * Recursively called for each element
   */
  template <std::size_t I,
            typename TupleType,
            typename = typename std::enable_if<
                I != std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type>
  void
  size_each(TupleType && t, std::vector<unsigned int> & sizes, std::integral_constant<size_t, I>)
  {
    sizes[I] = sizeOfData(std::get<I>(t));

    // Continue the recursion
    size_each(std::forward<TupleType>(t), sizes, std::integral_constant<size_t, I + 1>());
  }

  /**
   * The function that is actually called that starts the recursion
   */
  template <typename TupleType>
  void size_each(TupleType && t, std::vector<unsigned int> & sizes)
  {
    size_each(std::forward<TupleType>(t), sizes, std::integral_constant<size_t, 0>());
  }

  /**
   * Finds the size each column should be and set it in _column_sizes
   */
  void size_columns()
  {
    _column_sizes.resize(_num_columns);

    // Temporary for querying each row
    std::vector<unsigned int> column_sizes(_num_columns);

    // Start with the size of the headers
    for (unsigned int i = 0; i < _num_columns; i++)
      _column_sizes[i] = _headers[i].size();

    // Grab the size of each entry of each row and see if it's bigger
    for (auto & row : _data)
    {
      size_each(row, column_sizes);

      for (unsigned int i = 0; i < _num_columns; i++)
        _column_sizes[i] = std::max(_column_sizes[i], column_sizes[i]);
    }
  }

  /// The column headers
  std::vector<std::string> _headers;

  /// Number of columns in the table
  unsigned int _num_columns;

  /// Size of columns that we can't get the size of
  unsigned int _static_column_size;

  /// The actual data
  std::vector<DataTuple> _data;

  /// Holds the printable width of each column
  std::vector<unsigned int> _column_sizes;
};
