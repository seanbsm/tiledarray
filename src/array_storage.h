#ifndef TILEDARRAY_ARRAY_STORAGE_H__INCLUDED
#define TILEDARRAY_ARRAY_STORAGE_H__INCLUDED

//#include <error.h>
#include <range.h>
#include <madness_runtime.h>
//#include <array_util.h>
#include <permutation.h>
#include <key.h>
#include <Eigen/Core>
//#include <boost/array.hpp>
//#include <boost/iterator/filter_iterator.hpp>
#include <boost/scoped_array.hpp>
//#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility.hpp>
//#include <cstddef>
//#include <algorithm>
//#include <memory>
#include <numeric>
//#include <iterator>
//#include <stdexcept>

namespace TiledArray {

  // Forward declarations
  template <typename I, unsigned int DIM, typename Tag, typename CS>
  class ArrayCoordinate;
  template <unsigned int Level>
  class LevelTag;
  template <unsigned int DIM>
  class Permutation;
  template <typename T, unsigned int DIM, typename Tag, typename CS>
  class DenseArrayStorage;
  template <typename T, unsigned int DIM, typename Tag, typename CS>
  class DistributedArrayStorage;
  template <typename T, unsigned int DIM, typename Tag, typename CS>
  void swap(DenseArrayStorage<T, DIM, Tag, CS>&, DenseArrayStorage<T, DIM, Tag, CS>&);
  template <typename T, unsigned int DIM, typename Tag, typename CS>
  void swap(DistributedArrayStorage<T, DIM, Tag, CS>&, DistributedArrayStorage<T, DIM, Tag, CS>&);
  template <typename T, unsigned int DIM, typename Tag, typename CS>
  DenseArrayStorage<T,DIM,Tag,CS> operator ^(const Permutation<DIM>&, const DenseArrayStorage<T,DIM,Tag,CS>&);

  namespace detail {
    template<typename I, unsigned int DIM, typename CS>
    bool less(const boost::array<I,DIM>&, const boost::array<I,DIM>&);

    template <typename I, unsigned int DIM, typename Tag, typename CS>
    class ArrayDim;
    template <typename I, unsigned int DIM, typename Tag, typename CS>
    void swap(ArrayDim<I, DIM, Tag, CS>&, ArrayDim<I, DIM, Tag, CS>&);
  } // namespace detail

  namespace detail {

    /// ArrayStorage is the base class for other storage classes.

    /// ArrayStorage stores array dimensions and is used to calculate ordinal
    /// values. It contains no actual array data; that is for the derived
    /// classes to implement. The array origin is always zero for all dimensions.
    template <typename I, unsigned int DIM, typename Tag, typename CS = CoordinateSystem<DIM> >
    class ArrayDim {
    public:
      typedef ArrayDim<I, DIM, Tag, CS> ArrayDim_;
      typedef I ordinal_type;
      typedef I volume_type;
      typedef CS coordinate_system;
      typedef ArrayCoordinate<ordinal_type, DIM, Tag, coordinate_system> index_type;
      typedef boost::array<ordinal_type,DIM> size_array;

      static unsigned int dim() { return DIM; }
      static detail::DimensionOrderType  order() { return coordinate_system::dimension_order; }

      /// Default constructor. Constructs a 0 dimension array.
      ArrayDim() : size_(), weight_(), n_(0) { // no throw
        size_.assign(0);
        weight_.assign(0);
      }

      /// Constructs an array with dimensions of size.
      ArrayDim(const size_array& size) : // no throw
          size_(size), weight_(calc_weight_(size)), n_(detail::volume(size))
      { }

      /// Copy constructor
      ArrayDim(const ArrayDim& other) : // no throw
          size_(other.size_), weight_(other.weight_), n_(other.n_)
      { }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
      /// Move constructor
      ArrayDim(ArrayDim&& other) : // no throw
          size_(std::move(other.size_)), weight_(std::move(other.weight_)), n_(other.n_)
      { }
#endif // __GXX_EXPERIMENTAL_CXX0X__

      /// Destructor
      ~ArrayDim() { } // no throw

      /// Assignment operator
      ArrayDim_& operator =(const ArrayDim_& other) {
        size_ = other.size_;
        weight_ = other.weight_;
        n_ = other.n_;

        return *this;
      }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
      /// Assignment operator
      ArrayDim_& operator =(ArrayDim_&& other) {
        size_ = std::move(other.size_);
        weight_ = std::move(other.weight_);
        n_ = other.n_;

        return *this;
      }
#endif // __GXX_EXPERIMENTAL_CXX0X__

      /// Returns the size of the array.
      const size_array& size() const { return size_; } // no throw

      /// Returns the number of elements in the array.
      ordinal_type volume() const { return n_; } // no throw

      /// Returns the dimension weights for the array.
      const size_array& weight() const { return weight_; } // no throw


      /// Returns true if i is less than the number of elements in the array.
      bool includes(const ordinal_type i) const { // no throw
        return i < n_;
      }

      /// Returns true if i is less than the number of elements in the array.
      bool includes(const index_type& i) const { // no throw
        return detail::less<ordinal_type, DIM>(i.data(), size_);
      }

      /// computes an ordinal index for a given an index_type
      ordinal_type ordinal(const index_type& i) const {
        TA_ASSERT(includes(i), std::out_of_range,
            "Index is not included in the array range.");
        return ord(i);
      }

      /// Sets the size of object to the given size.
      void resize(const size_array& s) {
        size_ = s;
        weight_ = calc_weight_(s);
        n_ = detail::volume(s);
      }

      /// Helper functions that converts index_type to ordinal_type indexes.

      /// This function is overloaded so it can be called by template functions.
      /// No range checking is done. This function will not throw.
      ordinal_type ord(const index_type& i) const { // no throw
        return std::inner_product(i.begin(), i.end(), weight_.begin(), typename index_type::index(0));
      }

      ordinal_type ord(const ordinal_type i) const { return i; } // no throw


      /// Class wrapper function for detail::calc_weight() function.
      static size_array calc_weight_(const size_array& size) { // no throw
        size_array result;
        calc_weight(coordinate_system::begin(size), coordinate_system::end(size),
            coordinate_system::begin(result));
        return result;
      }

      friend void swap<>(ArrayDim_& first, ArrayDim_& second);

      size_array size_;
      size_array weight_;
      ordinal_type n_;
    }; // class ArrayDim

    template <typename I, unsigned int DIM, typename Tag, typename CS>
    void swap(ArrayDim<I, DIM, Tag, CS>& first, ArrayDim<I, DIM, Tag, CS>& second) {
      boost::swap(first.size_, second.size_);
      boost::swap(first.weight_, second.weight_);
      std::swap(first.n_, second.n_);
    }

  } // namespace detail

  /// DenseArrayStorage stores data for a dense N-dimensional Array. Data is
  /// stored in order in the order specified by the coordinate system template
  /// parameter. The default allocator used by array storage is std::allocator.
  /// All data is allocated and stored locally. Type T must be default-
  /// Constructible and copy-constructible. You may work around the default
  /// constructor requirement by specifying default values in
  template <typename T, unsigned int DIM, typename Tag = LevelTag<0>, typename CS = CoordinateSystem<DIM> >
  class DenseArrayStorage {
  private:
    typedef Eigen::aligned_allocator<T> alloc_type;
  public:
    typedef DenseArrayStorage<T,DIM,Tag,CS> DenseArrayStorage_;
    typedef detail::ArrayDim<std::size_t, DIM, Tag, CS> array_dim_type;
    typedef typename array_dim_type::index_type index_type;
    typedef typename array_dim_type::ordinal_type ordinal_type;
    typedef typename array_dim_type::volume_type volume_type;
    typedef typename array_dim_type::size_array size_array;
    typedef T value_type;
    typedef CS coordinate_system;
    typedef Tag tag_type;
    typedef T * iterator;
    typedef const T * const_iterator;
    typedef T & reference_type;
    typedef const T & const_reference_type;

    static unsigned int dim() { return DIM; }
    static detail::DimensionOrderType  order() { return coordinate_system::dimension_order; }

    /// Default constructor.

    /// Constructs an empty array. You must call
    /// DenseArrayStorage::resize(const size_array&) before the array can be
    /// used.
    DenseArrayStorage() : dim_(), data_(NULL), alloc_() { }

    /// Constructs an array with dimensions of size and fill it with val.
    DenseArrayStorage(const size_array& size, const value_type& val = value_type()) :
        dim_(size), data_(NULL), alloc_()
    {
      create_(val);
    }

    /// Construct the array with the given data.

    /// Constructs an array of size and fills it with the data indicated by
    /// the first and last input iterators. The range of data [first, last)
    /// must point to a range at least as large as the array being constructed.
    /// If the iterator range is smaller than the array, the constructor will
    /// throw an assertion error.
    template <typename InIter>
    DenseArrayStorage(const size_array& size, InIter first, InIter last) :
        dim_(size), data_(NULL), alloc_()
    {
      create_(first, last);
    }

    /// Copy constructor

    /// The copy constructor performs a deep copy of the data.
    DenseArrayStorage(const DenseArrayStorage_& other) :
        dim_(other.dim_), data_(NULL), alloc_()
    {
      create_(other.begin(), other.end());
    }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    /// Move constructor
    DenseArrayStorage(DenseArrayStorage_&& other) : dim_(std::move(other.dim_)),
        data_(other.data_), alloc_()
    {
      other.data_ = NULL;
    }
#endif // __GXX_EXPERIMENTAL_CXX0X__

    /// Destructor
    ~DenseArrayStorage() {
      destroy_();
    }

    DenseArrayStorage_& operator =(const DenseArrayStorage_& other) {
      DenseArrayStorage_ temp(other);
      swap(*this, temp);

      return *this;
    }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    DenseArrayStorage_& operator =(DenseArrayStorage_&& other) {
      if(this != &other) {
        destroy_();
        dim_ = std::move(other.dim_);
        data_ = other.data_;
        other.data_ = NULL;
      }
      return *this;
    }
#endif // __GXX_EXPERIMENTAL_CXX0X__

    /// In place permutation operator.

    /// This function permutes its elements only.
    /// No assumptions are made about the data contained by this array.
    /// Therefore, if the data in each element of the array also needs to be
    /// permuted, it's up to the array owner to permute the data.
    DenseArrayStorage_& operator ^=(const Permutation<DIM>& p) {
      if(data_ != NULL) {
        DenseArrayStorage_ temp = p ^ (*this);
        swap(*this, temp);
      }
      return *this;
    }

    /// Resize the array. The current data common to both arrays is maintained.
    /// Any new elements added have be assigned a value of val. If val is not
    /// specified, the default constructor will be used for new elements.
    DenseArrayStorage_& resize(const size_array& size, value_type val = value_type()) {
      DenseArrayStorage_ temp(size, val);
      if(data_ != NULL) {
        // replace Range with ArrayDim?
        typedef Range<ordinal_type, DIM, Tag, coordinate_system > range_type;
        range_type range_temp(size);
        range_type range_curr(dim_.size_);
        range_type range_common = range_temp & range_curr;

        for(typename range_type::const_iterator it = range_common.begin(); it != range_common.end(); ++it)
          temp[ *it ] = operator[]( *it ); // copy common data.
      }
      swap(*this, temp);
      return *this;
    }

    /// Returns a raw pointer to the array elements. Elements are ordered from
    /// least significant to most significant dimension.
    value_type * data() { return data_; }

    /// Returns a constant raw pointer to the array elements. Elements are
    /// ordered from least significant to most significant dimension.
    const value_type * data() const { return data_; }

    // Iterator factory functions.
    iterator begin() { // no throw
      return data_;
    }

    iterator end() { // no throw
      return data_ + dim_.n_;
    }

    const_iterator begin() const { // no throw
      return data_;
    }

    const_iterator end() const { // no throw
      return data_ + dim_.n_;
    }

    /// Returns a reference to element i (range checking is performed).

    /// This function provides element access to the element located at index i.
    /// If i is not included in the range of elements, std::out_of_range will be
    /// thrown. Valid types for Index are ordinal_type and index_type.
    template <typename Index>
    reference_type at(const Index& i) {
      if(! dim_.includes(i))
        throw std::out_of_range("DenseArrayStorage<...>::at(...): Element is not in range.");

      return * (data_ + dim_.ord(i));
    }

    /// Returns a constant reference to element i (range checking is performed).

    /// This function provides element access to the element located at index i.
    /// If i is not included in the range of elements, std::out_of_range will be
    /// thrown. Valid types for Index are ordinal_type and index_type.
    template <typename Index>
    const_reference_type at(const Index& i) const {
      if(! dim_.includes(i))
        throw std::out_of_range("DenseArrayStorage<...>::at(...) const: Element is not in range.");

      return * (data_ + dim_.ord(i));
    }

    /// Returns a reference to the element at i.

    /// This No error checking is performed.
    template <typename Index>
    reference_type operator[](const Index& i) { // no throw for non-debug
#ifdef NDEBUG
      return * (data_ + dim_.ord(i));
#else
      return at(i);
#endif
    }

    /// Returns a constant reference to element i. No error checking is performed.
    template <typename Index>
    const_reference_type operator[](const Index& i) const { // no throw for non-debug
#ifdef NDEBUG
      return * (data_ + dim_.ord(i));
#else
      return at(i);
#endif
    }

    /// Return the sizes of each dimension.
    const size_array& size() const { return dim_.size(); }

    /// Returns the dimension weights.

    /// The dimension weights are used to calculate ordinal values and is useful
    /// for determining array boundaries.
    const size_array& weight() const { return dim_.weight(); }

    /// Returns the number of elements in the array.
    volume_type volume() const { return dim_.volume(); }

    /// Returns true if the given index is included in the array.
    bool includes(const index_type& i) const { return dim_.includes(i); }

    /// Returns true if the given index is included in the array.
    bool includes(const ordinal_type& i) const { return dim_.includes(i); }

    /// Returns the ordinal (linearized) index for the given index.

    /// If the given index is not included in the
    ordinal_type ordinal(const index_type& i) const { return dim_.ordinal(i); }

  private:
    /// Allocate and initialize the array.

    /// All elements will contain the given value.
    void create_(const value_type val) {
      TA_ASSERT(data_ == NULL, std::runtime_error,
          "Cannot allocate data to a non-NULL pointer.");
      data_ = alloc_.allocate(dim_.n_);
      for(ordinal_type i = 0; i < dim_.n_; ++i)
        alloc_.construct(data_ + i, val);
    }

    /// Allocate and initialize the array.

    /// All elements will be initialized to the values given by the iterators.
    /// If the iterator range does not contain enough elements to fill the array,
    /// the remaining elements will be initialized with the default constructor.
    template <typename InIter>
    void create_(InIter first, InIter last) {
      TA_ASSERT(data_ == NULL, std::runtime_error,
          "Cannot allocate data to a non-NULL pointer.");
      data_ = alloc_.allocate(dim_.n_);
      ordinal_type i = 0;
      for(;first != last; ++first, ++i)
        alloc_.construct(data_ + i, *first);
      for(; i < dim_.n_; ++i)
        alloc_.construct(data_ + i, value_type());
    }

    /// Destroy the array
    void destroy_() {
      if(data_ != NULL) {
        value_type* d = data_;
        const value_type* const e = data_ + dim_.n_;
        for(; d != e; ++d)
          alloc_.destroy(d);

        alloc_.deallocate(data_, dim_.n_);
        data_ = NULL;
      }
    }

    friend void swap<>(DenseArrayStorage_& first, DenseArrayStorage_& second);

    array_dim_type dim_;
    value_type* data_;
    alloc_type alloc_;
  }; // class DenseArrayStorage

  /// Stores an n-dimensional array across many nodes.

  /// DistributedArrayStorage stores array elements on one or more nodes of a
  /// cluster. Some of the data may exist on the local node. This class assumes
  /// that the T represents a type with a large amount of data and therefore
  /// will store and retrieve them individually. All communication and data transfer
  /// is handled by the madness library. Iterators will only iterate over local
  /// data. If we were to allow iteration over all data, all data would be sent
  /// to the local node.
  template <typename T, unsigned int DIM, typename Tag = LevelTag<1>, typename CS = CoordinateSystem<DIM> >
  class DistributedArrayStorage {
  public:
    typedef DistributedArrayStorage<T, DIM, Tag, CS> DistributedArrayStorage_;
    typedef detail::ArrayDim<std::size_t, DIM, Tag, CS> array_dim_type;
    typedef typename array_dim_type::index_type index_type;
    typedef typename array_dim_type::ordinal_type ordinal_type;
    typedef typename array_dim_type::volume_type volume_type;
    typedef typename array_dim_type::size_array size_array;
    typedef CS coordinate_system;

    // Note: Since key_type is actually two keys, all elements inserted into
    // the data_container must include both key_types so the array can function
    // correctly when given an index_type, ordinal_type, or key_type.
    typedef detail::Key<ordinal_type, index_type> key_type;

  private:

    ///
    struct ArrayHash : public std::unary_function<key_type, madness::hashT> {
      ArrayHash(const array_dim_type& d) : dim_(&d) {}
      void set(const array_dim_type& d) { dim_ = &d; }
      madness::hashT operator()(const key_type& k) const {
        const typename array_dim_type::ordinal_type o =
            (k.keys() == 2 ? dim_->ord(k.key2()) : k.key1() );
        return madness::hash(o);
      }
    private:
      ArrayHash();
      const array_dim_type* dim_;
    }; // struct ArrayHash

//    typedef detail::ArrayHash<key_type, array_dim_type> hasher_type;
    typedef madness::WorldContainer<key_type, T, ArrayHash > data_container;

  public:
    typedef typename data_container::pairT value_type;

    typedef typename data_container::iterator iterator;
    typedef typename data_container::const_iterator const_iterator;
    typedef T & reference_type;
    typedef const T & const_reference_type;
    typedef typename data_container::accessor accessor;
    typedef typename data_container::const_accessor const_accessor;

    static unsigned int dim() { return DIM; }

  private:
    // Operations not permitted.
    DistributedArrayStorage();
    DistributedArrayStorage(const DistributedArrayStorage_& other);
    DistributedArrayStorage_& operator =(const DistributedArrayStorage_& other);

  public:
    /// Constructs a zero size array.
    /// Construct an array with a definite size. All data elements are
    /// uninitialized. No communication occurs.
    DistributedArrayStorage(madness::World& world) :
        dim_(), data_(world, true, ArrayHash(dim_))
    { }

    /// Construct an array with a definite size. All data elements are
    /// uninitialized. No communication occurs.
    DistributedArrayStorage(madness::World& world, const size_array& size) :
        dim_(size), data_(world, true, ArrayHash(dim_))
    { }

    /// Construct an array with a definite size and initializes the data.

    /// This constructor creates an array of size and fills in data with the
    /// list provided by the input iterators [first, last). It may be used to
    /// create a deep copy of another Distributed array. If store_local is
    /// true, only local data is stored. If store_local is false, all values
    /// will be stored. Non-local values will be sent to the owning process with
    /// non-blocking communication. store_local is true by default. You will
    /// want to set store_local to false when you are storing data where you do
    /// not know where the data will be stored.  InIter type must be an input
    /// iterator or compatible type and dereference to a
    /// std::pair<index_type,value_type> type.
    ///
    /// Caution: If you set store_local to false, make sure you do not assign
    /// duplicated values in different processes. If you do not excess
    /// communication my occur, which will negatively affect performance. In
    /// addition, if the dupicate values are different, there is no way to
    /// predict which one will be the final value.
    template <typename InIter>
    DistributedArrayStorage(madness::World& world, const size_array& size, InIter first, InIter last) :
        dim_(size), data_(world, false, ArrayHash(dim_))
    {
      for(;first != last; ++first)
        if(is_local(first->first))
          insert(first->first, first->second);

      data_.process_pending();
      data_.get_world().gop.barrier(); // Make sure everyone is done writing
                                       // before proceeding.
    }

    ~DistributedArrayStorage() { }

    /// Copy the content of this array into the other array.

    /// Performs a deep copy of this array into the other array. The content of
    /// the other array will be deleted. This function is blocking and may cause
    /// some communication.
    void clone(const DistributedArrayStorage_& other) {
      DistributedArrayStorage_ temp(data_.get_world(), other.dim_.size());
      temp.insert(other.begin(), other.end());
      data_.clear();
      swap(*this, temp);
      data_.get_world().gop.fence();
    }

    /// Inserts an element into the array

    /// Inserts a local element into the array. This will initiate non-blocking
    /// communication that will replace or insert in a remote location. Local
    /// element insertions with insert_remote() are equivilant to insert(). If
    /// the element is already present, the previous element will be destroyed.
    /// If the element is not in the range of for the array, a std::out_of_range
    /// exception will be thrown.
    template<typename Key>
    void insert(const Key& i, const_reference_type v) {
      TA_ASSERT(includes(i), std::out_of_range, "The index is not in range.");
      data_.replace(make_key_(i), v);
    }

    /// Inserts an element into the array

    /// Inserts a local element into the array. This will initiate non-blocking
    /// communication that will replace or insert in a remote location. Local
    /// element insertions with insert_remote() are equivilant to insert(). If
    /// the element is already present, the previous element will be destroyed.
    /// If the element is not in the range of for the array, a std::out_of_range
    /// exception will be thrown.
    template<typename Key>
    void insert(const std::pair<Key, T>& e) {
      insert(e.first, e.second);
    }

    template<typename InIter>
    void insert(InIter first, InIter last) {
      for(;first != last; ++first)
        insert(first->first, first->second);
    }

    /// Erases the element specified by the index

    /// This function removes the element specified by the index, and performs a
    /// non-blocking communication for non-local elements.
    template<typename Key>
    void erase(const Key& i) {
      data_.erase(key_(i));
    }

    /// Erase the range of iterators

    /// The iterator range must point to a list of pairs where std::pair::first
    /// is the index to be deleted. It is intended to be used with a range of
    /// element iterators.
    template<typename InIter>
    void erase(InIter first, InIter last) {
      for(; first != last; ++first)
        erase(first->first);
    }

    /// Erase all elements of the array.
    void clear() {
      data_.clear();
    }

    /// In place permutation operator.

    /// This function will permute the elements of the array. This function is a global sync point.
    DistributedArrayStorage_& operator ^=(const Permutation<DIM>& p) {
      typedef Range<ordinal_type, DIM, Tag, CS> range_type;

      /// Construct temporary container.
      range_type r(dim_.size());
      DistributedArrayStorage_ temp(data_.get_world(), p ^ (dim_.size()));

      // Iterate over all indices in the array. For each element d_.find() is
      // used to request data at the current index. If the data is  local, the
      // element is written into the temp array, otherwise it is skipped. When
      // the data is written, non-blocking communication may occur (when the new
      // location is not local).
      const_accessor a;
      key_type k;
      for(typename range_type::const_iterator it = r.begin(); it != r.end(); ++it) {
        k = dim_.ord(*it);
        if( data_.find(a, k)) {
          temp.insert(p ^ *it, a->second);
          a.release();
          data_.erase(k);
        }
      }

      // not communicate and should take no time
      swap(*this, temp);
      data_.get_world().gop.barrier();
      return *this;
    }

    /// Resize the array.

    /// This resize will maintain the data common to both arrays. Some
    /// non-blocking communication will likely occur. Any new elements added
    /// have uninitialized data. This function is a global sync point.
    DistributedArrayStorage_& resize(const size_array& size, bool keep_data = true) {
      typedef Range<ordinal_type, DIM, Tag, coordinate_system> range_type;

      /// Construct temporary container.
      DistributedArrayStorage temp(data_.get_world(), size);

      if(keep_data) {
        range_type common_rng(range_type(dim_.size_) & range_type(size));

        // Iterate over all indices in the array. For each element d_.find() is
        // used to request data at the current index. If the data is  local, the
        // element is written into the temp array, otherwise it is skipped. When
        // the data is written, non-blocking communication may occur (when the new
        // location is not local).
        typename data_container::const_accessor a;
        key_type k;
        for(typename range_type::const_iterator it = common_rng.begin(); it != common_rng.end(); ++it) {
          k = dim_.ord(*it);
          if( data_.find(a, k))
            temp.data_.replace(k, a->second);
          a.release();
        }

      }

      swap(*this, temp);

      data_.get_world().gop.fence(); // Make sure write is complete before proceeding.
      return *this;
    }

    /// Returns an iterator to the beginning local data.
    iterator begin() { return data_.begin(); }
    /// Returns an iterator to the end of the local data.
    iterator end() { return data_.end(); }
    /// Returns a constant iterator to the beginning of the local data.
    const_iterator begin() const { return data_.begin(); }
    /// Returns a constant iterator to the end of the local data.
    const_iterator end() const { return data_.end(); }

    /// Return the sizes of each dimension.
    const size_array& size() const { return dim_.size(); }

    /// Returns the dimension weights.

    /// The dimension weights are used to calculate ordinal values and is useful
    /// for determining array boundaries.
    const size_array& weight() const { return dim_.weight(); }

    /// Returns the number of elements in the array.
    volume_type volume(bool local = false) const { return ( local ? data_.size() : dim_.volume() ); }

    /// Returns true if the given index is included in the array.
    template<typename Key>
    bool includes(const Key& i) const { return dim_.includes(i); }

    /// Returns true if the given key is included in the array.
    bool includes(const key_type& k) const {
      TA_ASSERT((k.keys() & 3) != 0, std::runtime_error, "Key is not initialized.");

      if(k.keys() & 1)
        return dim_.includes(k.key1());

      return dim_.includes(k.key2());
    }

    /// Returns a Future iterator to an element at index i.

    /// This function will return an iterator to the element specified by index
    /// i. If the element is not local the it will use non-blocking communication
    /// to retrieve the data. The future will be immediately available if the data
    /// is local. Valid types for Index are ordinal_type or index_type.
    template<typename Key>
    madness::Future<iterator> find(const Key& i) {
      return data_.find(key_(i));
    }

    /// Returns a Future const_iterator to an element at index i.

    /// This function will return a const_iterator to the element specified by
    /// index i. If the element is not local the it will use non-blocking
    /// communication to retrieve the data. The future will be immediately
    /// available if the data is local. Valid types for Index are ordinal_type
    /// or index_type.
    template<typename Key>
    madness::Future<const_iterator> find(const Key& i) const {
      return data_.find(key_(i));
    }

    /// Sets an accessor to point to a local data element.

    /// This function will set an accessor to point to a local data element only.
    /// It will return false if the data element is remote or not found.
    template<typename Key>
    bool find(accessor& acc, const Key& i) {
      return data_.find(acc, key_(i));
    }

    /// Sets a const_accessor to point to a local data element.

    /// This function will set a const_accessor to point to a local data element
    /// only. It will return false if the data element is remote or not found.
    template<typename Key>
    bool find(const_accessor& acc, const Key& i) const {
      return data_.find(acc, key_(i));
    }

    /// Returns true if index i is stored locally.

    /// Note: This function does not check for the presence of an element at
    /// the given location. If the index is not included in the range, then the
    /// result will be erroneous.
    template<typename Key>
    bool is_local(const Key& i) const {
      return data_.is_local(key_(i));
    }

    madness::World& get_world() const {
      return data_.get_world();
    }

  private:

    /// Converts an ordinal into an index
    index_type get_index_(const ordinal_type i) const {
      index_type result;
      detail::calc_index(i, coordinate_system::rbegin(dim_.weight()),
          coordinate_system::rend(dim_.weight()),
          coordinate_system::rbegin(result));
      return result;
    }

    /// Returns the ordinal given a key
    ordinal_type ord_(const key_type& k) const {
      return k.key1();
    }

    /// Returns the ordinal given an index
    ordinal_type ord_(const index_type& i) const {
      return dim_.ord(i);
    }

    /// Returns the given ordinal
    ordinal_type ord_(const ordinal_type& i) const {
      return i;
    }

    /// Returns a key (key1_type)
    key_type key_(const ordinal_type& i) const {
      return key_type(i);
    }

    /// Returns a key (key1_type)
    key_type key_(const index_type& i) const {
      return key_type(dim_.ord(i));
    }

    /// Returns the given key
    key_type key_(const key_type& k) const {
      return k;
    }

    /// Returns a key that contains both key types, base on an index
    key_type make_key_(const index_type& i) const {
      return key_type(dim_.ord(i), i);
    }

    /// returns a key that contains both key types, pase on an ordinal
    key_type make_key_(const ordinal_type& i) const {
      return key_type(i, get_index_(i));
    }

    /// Returns the give key if b
    key_type make_key_(const key_type& k) const {
      TA_ASSERT( k.keys() & 3u , std::runtime_error, "No valid keys are assigned.");
      if(k.keys() == 3u)
        return k;

      key_type result((k.keys() & 1u ? k.key1() : ord_(k.key2())),
                      (k.keys() & 2u ? k.key2() : get_index_(k.key1())));
      return result;
    }

    friend void swap<>(DistributedArrayStorage_&, DistributedArrayStorage_&);

    array_dim_type dim_;
    data_container data_;
  }; // class DistributedArrayStorage

  /// Swap the data of the two arrays.
  template <typename T, unsigned int DIM, typename Tag, typename CS>
  void swap(DenseArrayStorage<T, DIM, Tag, CS>& first, DenseArrayStorage<T, DIM, Tag, CS>& second) { // no throw
    detail::swap(first.dim_, second.dim_);
    std::swap(first.data_, second.data_);
  }

  /// Swap the data of the two distributed arrays.
  template <typename T, unsigned int DIM, typename Tag, typename CS>
  void swap(DistributedArrayStorage<T, DIM, Tag, CS>& first, DistributedArrayStorage<T, DIM, Tag, CS>& second) {
    detail::swap(first.dim_, second.dim_);
    madness::swap(first.data_, second.data_);
    first.data_.get_hash().set(first.dim_);
    second.data_.get_hash().set(second.dim_);
  }

  /// Permutes the content of the n-dimensional array.
  template <typename T, unsigned int DIM, typename Tag, typename CS>
  DenseArrayStorage<T,DIM,Tag,CS> operator ^(const Permutation<DIM>& p, const DenseArrayStorage<T,DIM,Tag,CS>& s) {
    DenseArrayStorage<T,DIM,Tag,CS> result(p ^ s.size());
    detail::Permute<DenseArrayStorage<T,DIM,Tag,CS> > f_perm(s);
    f_perm(p, result.begin(), result.end());

    return result;
  }

} // namespace TiledArray


namespace madness {
  namespace archive {

    template <class Archive, typename T, unsigned int DIM, typename Tag, typename CS>
    struct ArchiveLoadImpl<Archive, TiledArray::DenseArrayStorage<T,DIM,Tag,CS> > {
      typedef TiledArray::DenseArrayStorage<T,DIM,Tag,CS> DAS;
      typedef typename DAS::value_type value_type;

      static inline void load(const Archive& ar, DAS& s) {
        typename DAS::size_array size;
        ar & size;
        std::size_t n = TiledArray::detail::volume(size);
        boost::scoped_array<value_type> data(new value_type[n]);
        ar & wrap(data.get(),n);
        DAS temp(size, data.get(), data.get() + n);

        TiledArray::swap(s, temp);
      }
    };

    template <class Archive, typename T, unsigned int DIM, typename Tag, typename CS>
    struct ArchiveStoreImpl<Archive, TiledArray::DenseArrayStorage<T,DIM,Tag,CS> > {
      typedef TiledArray::DenseArrayStorage<T,DIM,Tag,CS> DAS;
      typedef typename DAS::value_type value_type;

      static inline void store(const Archive& ar, const DAS& s) {
        ar & s.size();
        ar & wrap(s.begin(), s.volume());
      }
    };

  }
}
#endif // TILEDARRAY_ARRAY_STORAGE_H__INCLUDED
