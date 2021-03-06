/**\file*********************************************************************
 *                                                                     \brief
 *  27.6.2 Output streams [output.streams]
 *
 ****************************************************************************
 */
#ifndef NTL__STLX_SSTREAM
#define NTL__STLX_SSTREAM
#pragma once

#include "ios.hxx"
#include "istream.hxx"

namespace std {

/**\addtogroup  lib_input_output ******* 27 Input/output library [input.output]
  *@{*/

/**\addtogroup  lib_string_streams ***** 27.7 String-based streams
  *@{*/
  template <class charT, class traits, class Allocator>
  class basic_stringbuf;

  typedef basic_stringbuf<char> stringbuf;
  typedef basic_stringbuf<wchar_t> wstringbuf;

  template <class charT, class traits, class Allocator>
  class basic_istringstream;

  typedef basic_istringstream<char> istringstream;
  typedef basic_istringstream<wchar_t> wistringstream;

  template <class charT, class traits, class Allocator>
  class basic_ostringstream;

  typedef basic_ostringstream<char> ostringstream;
  typedef basic_ostringstream<wchar_t> wostringstream;

  template <class charT, class traits, class Allocator>
  class basic_stringstream;

  typedef basic_stringstream<char> stringstream;
  typedef basic_stringstream<wchar_t> wstringstream;


  /**
   *	@brief 27.7.1 Class template basic_stringbuf [stringbuf]
   *  @details The class basic_stringbuf is derived from basic_streambuf to associate
   *  possibly the input sequence and possibly the output sequence with a sequence of arbitrary characters.
   *  The sequence can be initialized from, or made available as, an object of class basic_string.
   **/
  template <class charT, class traits, class Allocator>
  class basic_stringbuf:
    public basic_streambuf<charT,traits>
  {
    typedef typename add_rvalue_reference<basic_stringbuf>::type rvalue;

    static const size_t initial_output_size = 64;
  private:
    basic_stringbuf(const basic_stringbuf& rhs) __deleted;
    basic_stringbuf&operator=(const basic_stringbuf& rhs) __deleted;
  public:
    ///\name Types
    typedef charT char_type;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;
    typedef traits traits_type;
    typedef Allocator allocator_type;

    ///\name 27.7.1.1 Constructors:
    explicit basic_stringbuf(ios_base::openmode which = ios_base::in | ios_base::out)
      :mode_(which)
    {}

    explicit basic_stringbuf(const basic_string<charT,traits,Allocator>& s, ios_base::openmode which = ios_base::in | ios_base::out)
      :mode_(which)
    {
      str(s);
    }

  #ifdef NTL_CXX_RV
    basic_stringbuf(basic_stringbuf&& rhs)
      :mode_()
    {
      swap(rhs);
    }
  #endif

    ///\name 27.7.1.2 Assign and swap:
  #ifdef NTL_CXX_RV
    basic_stringbuf& operator=(basic_stringbuf&& rhs)
    {
      swap(rhs); return *this;
    }
  #endif

    void swap(basic_stringbuf& rhs)
    {
      if(this != &rhs){
        using std::swap;
        basic_streambuf::swap(rhs);
        swap(str_, rhs.str_);
        swap(mode_, rhs.mode_);
      }
    }

    ///\name 27.7.1.3 Get and set:
    basic_string<charT,traits,Allocator> str() const
    {
      // NOTE: update to the specification:
      /*
       * If the basic_stringbuf was created only in input mode, the resultant basic_string
      contains the character sequence in the range [eback(),egptr()).
       * If the basic_stringbuf was created with which & ios_base::out being true
      then the resultant basic_string contains the character sequence in the range [pbase(),high_mark),
      where high_mark represents the position one past the highest initialized character in the buffer.
       */

      typedef basic_string<charT,traits,Allocator> string_type;

      if(mode_ & ios_base::out) {
        return string_type(this->pbase(), this->pptr(), str_.get_allocator());
      } else if(mode_ & ios_base::in) {
        return string_type(this->eback(), this->egptr(), str_.get_allocator());
      } else {
        return string_type(str_.get_allocator());
      }
    }

    void str(const basic_string<charT,traits,Allocator>& s)
    {
      //str_ = s;
      if(mode_ & ios_base::out){
        // reserve additional characters for output buffer
        growto(s.size() + initial_output_size);
      }
      str_.reserve(initial_output_size);
      str_.assign(s);
      #ifdef NTL_DEBUG
      str_.c_str(); // pretty view //-V530
      #endif
      set_ptrs();
      growto(str_.size());
      if(mode_ & ios_base::ate)
        pbump(static_cast<int>(epptr()-pptr()));
    }

  protected:
    ///\name 27.7.1.4 Overridden virtual functions:
    streamsize showmanyc()
    {
      streamsize c = -1;
      if(mode_ & ios_base::in){
        syncg();
        c = egptr()-gptr();
      }
      return c;
    }

    virtual int_type underflow()
    {
      syncg();
      return this->gptr() < this->egptr()
        ? traits::to_int_type(*this->gptr()) : traits::eof();
    }

    virtual int_type pbackfail(int_type c = traits::eof())
    {
      // backup sequence
      if ( this->eback() < this->gptr() )
      {
        if ( !traits::eq_int_type(c, traits::eof()) )
        {
          if ( !traits::eq(traits::to_char_type(c), this->gptr()[-1])
            && mode_ & ios_base::out )
            this->gptr()[-1] = traits::to_char_type(c);
        }
        this->gbump(-1);
        return traits::not_eof(c);
      }
      return traits::eof();
    }

    virtual int_type overflow (int_type c = traits::eof())
    {
      const int_type eof = traits::eof();
      if(traits::eq_int_type(c, eof))
        return eof;

      if(!(mode_ & ios_base::out))
        return eof;

      const char_type cc = traits::to_char_type(c);
      if(pptr() < epptr()){
        *pptr() = cc;
        pbump(1);
      }else{
        if(mode_ & ios_base::app)
          pbump(static_cast<int>(epptr()-pptr()));

        const ptrdiff_t gp = gptr() - eback(), pp = pptr() - pbase();

        growto(max(initial_output_size, __ntl_grow_heap_block_size(str_.size())));
        char_type* newbeg = str_.begin();
        setp(newbeg, newbeg+str_.capacity());
        pbump(static_cast<int>(pp)); // NOTE: there is an issue in "C++ Standard Library Issues List" about this

        // 27.8.1.4/8
        //if(mode_ & ios_base::in)
        setg(newbeg, newbeg+gp, pptr());
        *pptr() = cc;
        pbump(1);
      }
      return c;
    }

    virtual pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode which = ios_base::in | ios_base::out)
    {
      pos_type re = pos_type(off_type(-1));
      bool in = (which & mode_ & ios_base::in) != 0,
        out = (which & mode_ & ios_base::out) != 0;
      in &= !(which & ios_base::out);
      out&= !(which & ios_base::in);
      const bool both = in && out && way != ios_base::cur;

      const char_type* const beg = in ? eback() : pbase();
      if((beg || !off) && (in || out || both)){
        // updage egptr
        syncg();
        off_type newoff = off;
        if(way == ios_base::cur)
          newoff += gptr() - beg;
        else if(way == ios_base::end)
          newoff += egptr() - beg;

        if((in || both) && newoff >= 0 && egptr()-beg >= newoff){
          gbump(static_cast<int>((beg+newoff)-gptr()));
          re = pos_type(newoff);
        }
        if((out || both) && newoff >= 0 && egptr()-beg >= newoff){
          pbump(static_cast<int>((beg+newoff)-pptr()));
          re = pos_type(newoff);
        }
      }
      return re;
    }

    virtual pos_type seekpos(pos_type sp, ios_base::openmode which = ios_base::in | ios_base::out)
    {
    #if 1
      return seekoff(off_type(sp), ios_base::beg, which);
    #else
      const bool in = (which & ios_base::in) != 0, out = (which & ios_base::out) != 0;
      bool ok = false;
      if(sp != -1 && (in || out)){
        syncg();
        ok = true;
        if(in){
          if(sp >= 0 && sp < (egptr()-eback()))
            gbump(static_cast<int>( eback() - gptr() + static_cast<streamoff>(sp) ));
          else
            ok = false;
        }
        if(out){
          if(sp >= 0 && sp < (epptr()-pbase()))
            pbump(static_cast<int>(  pbase() - pptr() + static_cast<streamoff>(sp)));
          else
            ok = false;
        }
      }
      return ok ? sp : pos_type(off_type(-1));
    #endif
    }

    ///\}

  private:
    /// set stream poiners according to mode. \see str.
    void set_ptrs()
    {
      char_type* p = str_.begin();
      if ( mode_ & ios_base::out )
      {
        if(str_.empty())
          growto(initial_output_size); // characters by default
        this->setp(p, p + str_.capacity());

        if(!(mode_ & ios_base::in)){
          p += str_.size();
          this->setg(p, p, p);
          return;
        }
      }
      if ( mode_ & ios_base::in ) {
        this->setg(p, p, str_.end());
      }
    }

    void syncg()
    {
      char_type* const p = pptr();
      if(p && p > egptr()){
        if(mode_ & ios_base::in)
          setg(eback(), gptr(), p);
        else
          setg(p, p, p);
      }
    }

    void growto(streamsize newsize)
    {
      str_.resize(newsize, '\0');
      if(str_.size() < str_.capacity())
        str_.resize(str_.capacity());
    }
  private:
    basic_string<charT, traits> str_;
    ios_base::openmode mode_;
  };


  /**
   *	@brief 27.7.2 Class template basic_istringstream [istringstream]
   *  @details The class basic_istringstream<charT, traits, Allocator> supports reading objects of class
   *  basic_string<charT, traits, Allocator>. It uses a basic_stringbuf<charT, traits, Allocator> object
   *  to control the associated storage.
   **/
  template <class charT, class traits, class Allocator>
  class basic_istringstream:
    public basic_istream<charT,traits>
  {
    typedef typename add_rvalue_reference<basic_istringstream>::type rvalue;
  public:
    ///\name Types
    typedef charT char_type;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;
    typedef traits traits_type;
    typedef Allocator allocator_type;

    ///\name 27.7.2.1 Constructors:
    explicit basic_istringstream(ios_base::openmode which = ios_base::in)
      :basic_istream<charT,traits>(&sb), sb(which | ios_base::in)
    {}

    explicit basic_istringstream(const basic_string<charT,traits,Allocator>& str, ios_base::openmode which = ios_base::in)
      :basic_istream<charT,traits>(&sb), sb(str, which | ios_base::in)
    {}

  #ifdef NTL_CXX_RV
    basic_istringstream(basic_istringstream&& rhs)
      : basic_istream<charT,traits>(std::move(rhs))
      , sb(std::move(rhs.sb))
    {
      this->set_rdbuf(&sb);
    }

    ///\name 27.7.2.2 Assign and swap:
    basic_istringstream& operator=(basic_istringstream&& rhs)
    {
      basic_istream::operator =(std::move(rhs));
      sb = std::move(rhs.sb);
      return *this;
    }
  #endif

    void swap(basic_istringstream& rhs)
    {
      if(this != &rhs) {
        basic_istream::swap(rhs);
        sb.swap(rhs.sb);
      }
    }

    ///\name 27.7.2.3 Members:
    basic_stringbuf<charT,traits,Allocator>* rdbuf() const
    {
      return const_cast<basic_stringbuf<charT,traits,Allocator>*>(&sb);
    }

    basic_string<charT,traits,Allocator> str() const { return sb.str(); }
    void str(const basic_string<charT,traits,Allocator>& s) { sb.str(s); }

    ///\}
  private:
    basic_stringbuf<charT,traits,Allocator> sb;

    basic_istringstream(const basic_istringstream& rhs) __deleted;
    basic_istringstream& operator=(const basic_istringstream& rhs) __deleted;
  };



  /**
   *	@brief 27.7.3 Class basic_ostringstream [ostringstream]
   *  @details The class basic_ostringstream<charT, traits, Allocator> supports writing objects of class
   *  basic_string<charT, traits, Allocator>. It uses a basic_stringbuf object to control the associated storage.
   **/
  template <class charT, class traits, class Allocator>
  class basic_ostringstream:
    public basic_ostream<charT,traits>
  {
    typedef typename add_rvalue_reference<basic_ostringstream>::type rvalue;
  public:
    ///\name Types
    typedef charT char_type;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;
    typedef traits traits_type;
    typedef Allocator allocator_type;

    ///\name 27.7.3.1 Constructors/destructor:
    explicit basic_ostringstream(ios_base::openmode which = ios_base::out)
      :basic_ostream<charT,traits>(&sb), sb(which | ios_base::out)
    {}

    explicit basic_ostringstream(const basic_string<charT,traits,Allocator>& str, ios_base::openmode which = ios_base::out)
      : basic_ostream<charT,traits>(&sb), sb(str, which | ios_base::out)
    {}

  #ifdef NTL_CXX_RV
    basic_ostringstream(basic_ostringstream&& rhs)
      : basic_ostream<charT,traits>(std::move(rhs))
      , sb(std::move(rhs.sb))
    {
      this->set_rdbuf(&sb);
    }

    ///\name 27.7.3.2 Assign/swap:
    basic_ostringstream& operator=(basic_ostringstream&& rhs)
    {
      basic_ostream::operator =(std::move(rhs));
      sb = std::move(rhs.sb);
      return *this;
    }
  #endif

    void swap(basic_ostringstream& rhs)
    {
      if(this != &rhs) {
        basic_ostream::swap(rhs);
        sb.swap(rhs.sb);
      }
    }

    ///\name 27.7.3.3 Members:
    basic_stringbuf<charT,traits,Allocator>* rdbuf() const
    {
      return const_cast<basic_stringbuf<charT,traits,Allocator>*>(&sb);
    }
    basic_string<charT,traits,Allocator> str() const { return sb.str(); }
    void str(const basic_string<charT,traits,Allocator>& s) { sb.str(s); }
    ///\}
  private:
    basic_stringbuf<charT,traits,Allocator> sb;

    basic_ostringstream(const basic_ostringstream& rhs) __deleted;
    basic_ostringstream& operator=(const basic_ostringstream& rhs) __deleted;
  };



  /**
   *	@brief 27.7.1 Class template basic_stringstream [stringstream]
   *  @details The class template basic_stringstream<charT, traits> supports reading and writing from objects of
   *  class basic_string<charT, traits, Allocator>. It uses a basic_stringbuf<charT, traits, Allocator>
   *  object to control the associated sequence.
   **/
  template <class charT, class traits, class Allocator>
  class basic_stringstream:
    public basic_iostream<charT,traits>
  {
    typedef typename add_rvalue_reference<basic_stringstream>::type rvalue;
    typedef basic_iostream<charT, traits> base_type;
  public:
    ///\name Types
    typedef charT char_type;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;
    typedef traits traits_type;
    typedef Allocator allocator_type;

    ///\name constructors/destructors
    explicit basic_stringstream(ios_base::openmode which = ios_base::out|ios_base::in)
      :base_type(&sb),  sb(which)
    {}

    explicit basic_stringstream(const basic_string<charT,traits,Allocator>& str, ios_base::openmode which = ios_base::out|ios_base::in)
      :base_type(&sb), sb(str, which)
    {}

  #ifdef NTL_CXX_RV
    basic_stringstream(basic_stringstream&& rhs)
      : base_type(std::move(rhs))
      , sb(std::move(rhs.sb))
    {
      this->set_rdbuf(&sb);
    }
  #endif

    ///\name 27.7.5.1 Assign/swap:
  #ifdef NTL_CXX_RV
    basic_stringstream& operator=(basic_stringstream&& rhs)
    {
      base_type::operator =(std::move(rhs));
      sb = std::move(rhs.sb);
      return *this;
    }
  #endif

    void swap(basic_stringstream& rhs)
    {
      if(this != &rhs) {
        base_type::swap(rhs);
        sb.swap(rhs.sb);
      }
    }

    ///\name Members:
    basic_stringbuf<charT,traits,Allocator>* rdbuf() const { return const_cast<basic_stringbuf<charT,traits,Allocator>*>(&sb); }
    basic_string<charT,traits,Allocator> str() const { return sb.str(); }
    void str(const basic_string<charT,traits,Allocator>& str) { sb.str(str); }
    ///\}
  private:
     basic_stringbuf<charT, traits> sb;

     basic_stringstream(const basic_stringstream& rhs) __deleted;
     basic_stringstream& operator=(const basic_stringstream& rhs) __deleted;
  };


  ///\name Swap functions
  template <class charT, class traits, class Allocator>
  inline void swap(basic_stringbuf    <charT, traits, Allocator>& x, basic_stringbuf    <charT, traits, Allocator>& y) { x.swap(y); }
  template <class charT, class traits, class Allocator>
  inline void swap(basic_istringstream<charT, traits, Allocator>& x, basic_istringstream<charT, traits, Allocator>& y) { x.swap(y); }
  template <class charT, class traits, class Allocator>
  inline void swap(basic_ostringstream<charT, traits, Allocator>& x, basic_ostringstream<charT, traits, Allocator>& y) { x.swap(y); }
  template <class charT, class traits, class Allocator>
  inline void swap(basic_stringstream <charT, traits, Allocator>& x, basic_stringstream <charT, traits, Allocator>& y) { x.swap(y); }

  ///\}
  /**@} lib_string_streams */
  /**@} lib_input_output */
} // std

#endif // NTL__STLX_SSTREAM
