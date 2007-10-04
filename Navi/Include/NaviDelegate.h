/*
	Efficient delegates in C++ that generate only two lines of asm code!
	http://www.codeproject.com/cpp/FastDelegate.asp
	Author: Don Clugston, major contributions were made by Jody Hagins.
	Current Version: 30-Mar-05 v1.5
	Modified for explicit use with NaviLibrary
*/

#ifndef __NaviDelegate_H__
#define __NaviDelegate_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include "NaviPlatform.h"
#include <memory.h> // to allow <,> comparisons

#define FASTDELEGATE_USESTATICFUNCTIONHACK

#if defined(_MSC_VER) && !defined(__MWERKS__) && !defined(__VECTOR_C) && !defined(__ICL) && !defined(__BORLANDC__)
#define FASTDLGT_ISMSVC

#if (_MSC_VER <1300) // Many workarounds are required for VC6.
#define FASTDLGT_VC6
#pragma warning(disable:4786) // disable this ridiculous warning
#endif

#endif

#if defined(_MSC_VER) && !defined(__MWERKS__)
#define FASTDLGT_MICROSOFT_MFP

#if !defined(__VECTOR_C)
// CodePlay doesn't have the __single/multi/virtual_inheritance keywords
#define FASTDLGT_HASINHERITANCE_KEYWORDS
#endif
#endif

// Does it allow function declarator syntax? The following compilers are known to work:
#if defined(FASTDLGT_ISMSVC) && (_MSC_VER >=1310) // VC 7.1
#define FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX
#endif

// Gcc(2.95+), and versions of Digital Mars, Intel and Comeau in common use.
#if defined (__DMC__) || defined(__GNUC__) || defined(__ICL) || defined(__COMO__)
#define FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX
#endif

// It works on Metrowerks MWCC 3.2.2. From boost.Config it should work on earlier ones too.
#if defined (__MWERKS__)
#define FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX
#endif

#ifdef __GNUC__ // Workaround GCC bug #8271 
	// At present, GCC doesn't recognize constness of MFPs in templates
#define FASTDELEGATE_GCC_BUG_8271
#endif



////////////////////////////////////////////////////////////////////////////////
//						General tricks used in this code
//
// (a) Error messages are generated by typdefing an array of negative size to
//     generate compile-time errors.
// (b) Warning messages on MSVC are generated by declaring unused variables, and
//	    enabling the "variable XXX is never used" warning.
// (c) Unions are used in a few compiler-specific cases to perform illegal casts.
// (d) For Microsoft and Intel, when adjusting the 'this' pointer, it's cast to
//     (char *) first to ensure that the correct number of *bytes* are added.
//
////////////////////////////////////////////////////////////////////////////////
//						Helper templates
//
////////////////////////////////////////////////////////////////////////////////


namespace NaviLibrary {
namespace detail {	// we'll hide the implementation details in a nested namespace.

template <class OutputClass, class InputClass>
inline OutputClass implicit_cast(InputClass input){
	return input;
}

template <class OutputClass, class InputClass>
union horrible_union{
	OutputClass out;
	InputClass in;
};

template <class OutputClass, class InputClass>
inline OutputClass horrible_cast(const InputClass input){
	horrible_union<OutputClass, InputClass> u;
	// Cause a compile-time error if in, out and u are not the same size.
	// If the compile fails here, it means the compiler has peculiar
	// unions which would prevent the cast from working.
	typedef int ERROR_CantUseHorrible_cast[sizeof(InputClass)==sizeof(u) 
		&& sizeof(InputClass)==sizeof(OutputClass) ? 1 : -1];
	u.in = input;
	return u.out;
}

////////////////////////////////////////////////////////////////////////////////
//						Workarounds
//
////////////////////////////////////////////////////////////////////////////////

// Backwards compatibility: This macro used to be necessary in the virtual inheritance
// case for Intel and Microsoft. Now it just forward-declares the class.
#define FASTDELEGATEDECLARE(CLASSNAME)	class CLASSNAME;

// Prevent use of the static function hack with the DOS medium model.
#ifdef __MEDIUM__
#undef FASTDELEGATE_USESTATICFUNCTIONHACK
#endif

#ifdef FASTDLGT_VC6
// VC6 workaround
typedef const void * DefaultVoid;
#else
// On any other compiler, just use a normal void.
typedef void DefaultVoid;
#endif

// Translate from 'DefaultVoid' to 'void'.
// Everything else is unchanged
template <class T>
struct DefaultVoidToVoid { typedef T type; };

template <>
struct DefaultVoidToVoid<DefaultVoid> {	typedef void type; };

// Translate from 'void' into 'DefaultVoid'
// Everything else is unchanged
template <class T>
struct VoidToDefaultVoid { typedef T type; };

template <>
struct VoidToDefaultVoid<void> { typedef DefaultVoid type; };



////////////////////////////////////////////////////////////////////////////////
//						Fast Delegates, part 1:
//
//		Conversion of member function pointer to a standard form
//
////////////////////////////////////////////////////////////////////////////////

#ifdef  FASTDLGT_MICROSOFT_MFP

#ifdef FASTDLGT_HASINHERITANCE_KEYWORDS

	class __single_inheritance GenericClass;
#endif
	class GenericClass {};
#else
	class GenericClass;
#endif

// The size of a single inheritance member function pointer.
const int SINGLE_MEMFUNCPTR_SIZE = sizeof(void (GenericClass::*)());

template <int N>
struct SimplifyMemFunc {
	template <class X, class XFuncType, class GenericMemFuncType>
	inline static GenericClass *Convert(X *pthis, XFuncType function_to_bind, 
		GenericMemFuncType &bound_func) { 
		// Unsupported member function type -- force a compile failure.
	    // (it's illegal to have a array with negative size).
		typedef char ERROR_Unsupported_member_function_pointer_on_this_compiler[N-100];
		return 0; 
	}
};

// For compilers where all member func ptrs are the same size, everything goes here.
// For non-standard compilers, only single_inheritance classes go here.
template <>
struct SimplifyMemFunc<SINGLE_MEMFUNCPTR_SIZE>  {	
	template <class X, class XFuncType, class GenericMemFuncType>
	inline static GenericClass *Convert(X *pthis, XFuncType function_to_bind, 
			GenericMemFuncType &bound_func) {
#if defined __DMC__  
		// Digital Mars doesn't allow you to cast between abitrary PMF's, 
		// even though the standard says you can. The 32-bit compiler lets you
		// static_cast through an int, but the DOS compiler doesn't.
		bound_func = horrible_cast<GenericMemFuncType>(function_to_bind);
#else 
        bound_func = reinterpret_cast<GenericMemFuncType>(function_to_bind);
#endif
        return reinterpret_cast<GenericClass *>(pthis);
	}
};

////////////////////////////////////////////////////////////////////////////////
//						Fast Delegates, part 1b:
//
//					Workarounds for Microsoft and Intel
//
////////////////////////////////////////////////////////////////////////////////

#ifdef FASTDLGT_MICROSOFT_MFP

template<>
struct SimplifyMemFunc< SINGLE_MEMFUNCPTR_SIZE + sizeof(int) >  {
	template <class X, class XFuncType, class GenericMemFuncType>
	inline static GenericClass *Convert(X *pthis, XFuncType function_to_bind, 
		GenericMemFuncType &bound_func) { 
		// We need to use a horrible_cast to do this conversion.
		// In MSVC, a multiple inheritance member pointer is internally defined as:
        union {
			XFuncType func;
			struct {	 
				GenericMemFuncType funcaddress; // points to the actual member function
				int delta;	     // #BYTES to be added to the 'this' pointer
			}s;
        } u;
		// Check that the horrible_cast will work
		typedef int ERROR_CantUsehorrible_cast[sizeof(function_to_bind)==sizeof(u.s)? 1 : -1];
        u.func = function_to_bind;
		bound_func = u.s.funcaddress;
		return reinterpret_cast<GenericClass *>(reinterpret_cast<char *>(pthis) + u.s.delta); 
	}
};

struct MicrosoftVirtualMFP {
	void (GenericClass::*codeptr)(); // points to the actual member function
	int delta;		// #bytes to be added to the 'this' pointer
	int vtable_index; // or 0 if no virtual inheritance
};

struct GenericVirtualClass : virtual public GenericClass
{
	typedef GenericVirtualClass * (GenericVirtualClass::*ProbePtrType)();
	GenericVirtualClass * GetThis() { return this; }
};

// __virtual_inheritance classes go here
template <>
struct SimplifyMemFunc<SINGLE_MEMFUNCPTR_SIZE + 2*sizeof(int) >
{

	template <class X, class XFuncType, class GenericMemFuncType>
	inline static GenericClass *Convert(X *pthis, XFuncType function_to_bind, 
		GenericMemFuncType &bound_func) {
		union {
			XFuncType func;
			GenericClass* (X::*ProbeFunc)();
			MicrosoftVirtualMFP s;
		} u;
		u.func = function_to_bind;
		bound_func = reinterpret_cast<GenericMemFuncType>(u.s.codeptr);
		union {
			GenericVirtualClass::ProbePtrType virtfunc;
			MicrosoftVirtualMFP s;
		} u2;
		// Check that the horrible_cast<>s will work
		typedef int ERROR_CantUsehorrible_cast[sizeof(function_to_bind)==sizeof(u.s)
			&& sizeof(function_to_bind)==sizeof(u.ProbeFunc)
			&& sizeof(u2.virtfunc)==sizeof(u2.s) ? 1 : -1];
   // Unfortunately, taking the address of a MF prevents it from being inlined, so 
   // this next line can't be completely optimised away by the compiler.
		u2.virtfunc = &GenericVirtualClass::GetThis;
		u.s.codeptr = u2.s.codeptr;
		return (pthis->*u.ProbeFunc)();
	}
};

#if (_MSC_VER >1300)

template <>
struct SimplifyMemFunc<SINGLE_MEMFUNCPTR_SIZE + 3*sizeof(int) >
{
	template <class X, class XFuncType, class GenericMemFuncType>
	inline static GenericClass *Convert(X *pthis, XFuncType function_to_bind, 
			GenericMemFuncType &bound_func) {
		// The member function pointer is 16 bytes long. We can't use a normal cast, but
		// we can use a union to do the conversion.
		union {
			XFuncType func;
			// In VC++ and ICL, an unknown_inheritance member pointer 
			// is internally defined as:
			struct {
				GenericMemFuncType m_funcaddress; // points to the actual member function
				int delta;		// #bytes to be added to the 'this' pointer
				int vtordisp;		// #bytes to add to 'this' to find the vtable
				int vtable_index; // or 0 if no virtual inheritance
			} s;
		} u;
		// Check that the horrible_cast will work
		typedef int ERROR_CantUsehorrible_cast[sizeof(XFuncType)==sizeof(u.s)? 1 : -1];
		u.func = function_to_bind;
		bound_func = u.s.funcaddress;
		int virtual_delta = 0;
		if (u.s.vtable_index) { // Virtual inheritance is used
			// First, get to the vtable. 
			// It is 'vtordisp' bytes from the start of the class.
			const int * vtable = *reinterpret_cast<const int *const*>(
				reinterpret_cast<const char *>(pthis) + u.s.vtordisp );

			// 'vtable_index' tells us where in the table we should be looking.
			virtual_delta = u.s.vtordisp + *reinterpret_cast<const int *>( 
				reinterpret_cast<const char *>(vtable) + u.s.vtable_index);
		}
		// The int at 'virtual_delta' gives us the amount to add to 'this'.
        // Finally we can add the three components together. Phew!
        return reinterpret_cast<GenericClass *>(
			reinterpret_cast<char *>(pthis) + u.s.delta + virtual_delta);
	};
};
#endif // MSVC 7 and greater

#endif // MS/Intel hacks

}  // namespace detail

////////////////////////////////////////////////////////////////////////////////
//						Fast Delegates, part 2:
//
//	Define the delegate storage, and cope with static functions
//
////////////////////////////////////////////////////////////////////////////////

class DelegateMemento {
protected: 
	// the data is protected, not private, because many
	// compilers have problems with template friends.
	typedef void (detail::GenericClass::*GenericMemFuncType)(); // arbitrary MFP.
	detail::GenericClass *m_pthis;
	GenericMemFuncType m_pFunction;

#if !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)
	typedef void (*GenericFuncPtr)(); // arbitrary code pointer
	GenericFuncPtr m_pStaticFunction;
#endif

public:
#if !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)
	DelegateMemento() : m_pthis(0), m_pFunction(0), m_pStaticFunction(0) {};
	void clear() {
		m_pthis=0; m_pFunction=0; m_pStaticFunction=0;
	}
#else
	DelegateMemento() : m_pthis(0), m_pFunction(0) {};
	void clear() {	m_pthis=0; m_pFunction=0;	}
#endif
public:
#if !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)
	inline bool IsEqual (const DelegateMemento &x) const{
	    // We have to cope with the static function pointers as a special case
		if (m_pFunction!=x.m_pFunction) return false;
		// the static function ptrs must either both be equal, or both be 0.
		if (m_pStaticFunction!=x.m_pStaticFunction) return false;
		if (m_pStaticFunction!=0) return m_pthis==x.m_pthis;
		else return true;
	}
#else // Evil Method
	inline bool IsEqual (const DelegateMemento &x) const{
		return m_pthis==x.m_pthis && m_pFunction==x.m_pFunction;
	}
#endif
	// Provide a strict weak ordering for DelegateMementos.
	inline bool IsLess(const DelegateMemento &right) const {
		// deal with static function pointers first
#if !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)
		if (m_pStaticFunction !=0 || right.m_pStaticFunction!=0) 
				return m_pStaticFunction < right.m_pStaticFunction;
#endif
		if (m_pthis !=right.m_pthis) return m_pthis < right.m_pthis;
	// There are no ordering operators for member function pointers, 
	// but we can fake one by comparing each byte. The resulting ordering is
	// arbitrary (and compiler-dependent), but it permits storage in ordered STL containers.
		return memcmp(&m_pFunction, &right.m_pFunction, sizeof(m_pFunction)) < 0;

	}
	// BUGFIX (Mar 2005):
	// We can't just compare m_pFunction because on Metrowerks,
	// m_pFunction can be zero even if the delegate is not empty!
	inline bool operator ! () const		// Is it bound to anything?
	{ return m_pthis==0 && m_pFunction==0; }
	inline bool empty() const		// Is it bound to anything?
	{ return m_pthis==0 && m_pFunction==0; }
public:
	DelegateMemento & operator = (const DelegateMemento &right)  {
		SetMementoFrom(right); 
		return *this;
	}
	inline bool operator <(const DelegateMemento &right) {
		return IsLess(right);
	}
	inline bool operator >(const DelegateMemento &right) {
		return right.IsLess(*this);
	}
	DelegateMemento (const DelegateMemento &right)  : 
		m_pFunction(right.m_pFunction), m_pthis(right.m_pthis)
#if !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)
		, m_pStaticFunction (right.m_pStaticFunction)
#endif
		{}
protected:
	void SetMementoFrom(const DelegateMemento &right)  {
		m_pFunction = right.m_pFunction;
		m_pthis = right.m_pthis;
#if !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)
		m_pStaticFunction = right.m_pStaticFunction;
#endif
	}
};


//						ClosurePtr<>
//
// A private wrapper class that adds function signatures to DelegateMemento.
// It's the class that does most of the actual work.
// The signatures are specified by:
// GenericMemFunc: must be a type of GenericClass member function pointer. 
// StaticFuncPtr:  must be a type of function pointer with the same signature 
//                 as GenericMemFunc.
// UnvoidStaticFuncPtr: is the same as StaticFuncPtr, except on VC6
//                 where it never returns void (returns DefaultVoid instead).

// An outer class, FastDelegateN<>, handles the invoking and creates the
// necessary typedefs.
// This class does everything else.

namespace detail {

template < class GenericMemFunc, class StaticFuncPtr, class UnvoidStaticFuncPtr>
class ClosurePtr : public DelegateMemento {
public:
	// These functions are for setting the delegate to a member function.

	// Here's the clever bit: we convert an arbitrary member function into a 
	// standard form. XMemFunc should be a member function of class X, but I can't 
	// enforce that here. It needs to be enforced by the wrapper class.
	template < class X, class XMemFunc >
	inline void bindmemfunc(X *pthis, XMemFunc function_to_bind ) {
		m_pthis = SimplifyMemFunc< sizeof(function_to_bind) >
			::Convert(pthis, function_to_bind, m_pFunction);
#if !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)
		m_pStaticFunction = 0;
#endif
	}
	// For const member functions, we only need a const class pointer.
	// Since we know that the member function is const, it's safe to 
	// remove the const qualifier from the 'this' pointer with a const_cast.
	// VC6 has problems if we just overload 'bindmemfunc', so we give it a different name.
	template < class X, class XMemFunc>
	inline void bindconstmemfunc(const X *pthis, XMemFunc function_to_bind) {
		m_pthis= SimplifyMemFunc< sizeof(function_to_bind) >
			::Convert(const_cast<X*>(pthis), function_to_bind, m_pFunction);
#if !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)
		m_pStaticFunction = 0;
#endif
	}
#ifdef FASTDELEGATE_GCC_BUG_8271	// At present, GCC doesn't recognize constness of MFPs in templates
	template < class X, class XMemFunc>
	inline void bindmemfunc(const X *pthis, XMemFunc function_to_bind) {
		bindconstmemfunc(pthis, function_to_bind);
#if !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)
		m_pStaticFunction = 0;
#endif
	}
#endif
	// These functions are required for invoking the stored function
	inline GenericClass *GetClosureThis() const { return m_pthis; }
	inline GenericMemFunc GetClosureMemPtr() const { return reinterpret_cast<GenericMemFunc>(m_pFunction); }

// There are a few ways of dealing with static function pointers.
// There's a standard-compliant, but tricky method.
// There's also a straightforward hack, that won't work on DOS compilers using the
// medium memory model. It's so evil that I can't recommend it, but I've
// implemented it anyway because it produces very nice asm code.

#if !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)

//				ClosurePtr<> - Safe version
//
// This implementation is standard-compliant, but a bit tricky.
// I store the function pointer inside the class, and the delegate then
// points to itself. Whenever the delegate is copied, these self-references
// must be transformed, and this complicates the = and == operators.
public:
	// The next two functions are for operator ==, =, and the copy constructor.
	// We may need to convert the m_pthis pointers, so that
	// they remain as self-references.
	template< class DerivedClass >
	inline void CopyFrom (DerivedClass *pParent, const DelegateMemento &x) {
		SetMementoFrom(x);
		if (m_pStaticFunction!=0) {
			// transform self references...
			m_pthis=reinterpret_cast<GenericClass *>(pParent);
		}
	}
	// For static functions, the 'static_function_invoker' class in the parent 
	// will be called. The parent then needs to call GetStaticFunction() to find out 
	// the actual function to invoke.
	template < class DerivedClass, class ParentInvokerSig >
	inline void bindstaticfunc(DerivedClass *pParent, ParentInvokerSig static_function_invoker, 
				StaticFuncPtr function_to_bind ) {
		if (function_to_bind==0) { // cope with assignment to 0
			m_pFunction=0;
		} else { 
			bindmemfunc(pParent, static_function_invoker);
        }
		m_pStaticFunction=reinterpret_cast<GenericFuncPtr>(function_to_bind);
	}
	inline UnvoidStaticFuncPtr GetStaticFunction() const { 
		return reinterpret_cast<UnvoidStaticFuncPtr>(m_pStaticFunction); 
	}
#else

//				ClosurePtr<> - Evil version
//
// For compilers where data pointers are at least as big as code pointers, it is 
// possible to store the function pointer in the this pointer, using another 
// horrible_cast. Invocation isn't any faster, but it saves 4 bytes, and
// speeds up comparison and assignment. If C++ provided direct language support
// for delegates, they would produce asm code that was almost identical to this.
// Note that the Sun C++ and MSVC documentation explicitly state that they 
// support static_cast between void * and function pointers.

	template< class DerivedClass >
	inline void CopyFrom (DerivedClass *pParent, const DelegateMemento &right) {
		SetMementoFrom(right);
	}
	// For static functions, the 'static_function_invoker' class in the parent 
	// will be called. The parent then needs to call GetStaticFunction() to find out 
	// the actual function to invoke.
	// ******** EVIL, EVIL CODE! *******
	template < 	class DerivedClass, class ParentInvokerSig>
	inline void bindstaticfunc(DerivedClass *pParent, ParentInvokerSig static_function_invoker, 
				StaticFuncPtr function_to_bind) {
		if (function_to_bind==0) { // cope with assignment to 0
			m_pFunction=0;
		} else { 
		   // We'll be ignoring the 'this' pointer, but we need to make sure we pass
		   // a valid value to bindmemfunc().
			bindmemfunc(pParent, static_function_invoker);
        }

		// WARNING! Evil hack. We store the function in the 'this' pointer!
		// Ensure that there's a compilation failure if function pointers 
		// and data pointers have different sizes.
		// If you get this error, you need to #undef FASTDELEGATE_USESTATICFUNCTIONHACK.
		typedef int ERROR_CantUseEvilMethod[sizeof(GenericClass *)==sizeof(function_to_bind) ? 1 : -1];
		m_pthis = horrible_cast<GenericClass *>(function_to_bind);
		// MSVC, SunC++ and DMC accept the following (non-standard) code:
//		m_pthis = static_cast<GenericClass *>(static_cast<void *>(function_to_bind));
		// BCC32, Comeau and DMC accept this method. MSVC7.1 needs __int64 instead of long
//		m_pthis = reinterpret_cast<GenericClass *>(reinterpret_cast<long>(function_to_bind));
	}
	// ******** EVIL, EVIL CODE! *******
	// This function will be called with an invalid 'this' pointer!!
	// We're just returning the 'this' pointer, converted into
	// a function pointer!
	inline UnvoidStaticFuncPtr GetStaticFunction() const {
		// Ensure that there's a compilation failure if function pointers 
		// and data pointers have different sizes.
		// If you get this error, you need to #undef FASTDELEGATE_USESTATICFUNCTIONHACK.
		typedef int ERROR_CantUseEvilMethod[sizeof(UnvoidStaticFuncPtr)==sizeof(this) ? 1 : -1];
		return horrible_cast<UnvoidStaticFuncPtr>(this);
	}
#endif // !defined(FASTDELEGATE_USESTATICFUNCTIONHACK)

	// Does the closure contain this static function?
	inline bool IsEqualToStaticFuncPtr(StaticFuncPtr funcptr){
		if (funcptr==0) return empty(); 
	// For the Evil method, if it doesn't actually contain a static function, this will return an arbitrary
	// value that is not equal to any valid function pointer.
		else return funcptr==reinterpret_cast<StaticFuncPtr>(GetStaticFunction());
	}
};


} // namespace detail

////////////////////////////////////////////////////////////////////////////////
//						Fast Delegates, part 3:
//
//				Wrapper classes to ensure type safety
//
////////////////////////////////////////////////////////////////////////////////

//N=1
template<class Param1, class RetType=detail::DefaultVoid>
class FastDelegate1 {
private:
	typedef typename detail::DefaultVoidToVoid<RetType>::type DesiredRetType;
	typedef DesiredRetType (*StaticFunctionPtr)(Param1 p1);
	typedef RetType (*UnvoidStaticFunctionPtr)(Param1 p1);
	typedef RetType (detail::GenericClass::*GenericMemFn)(Param1 p1);
	typedef detail::ClosurePtr<GenericMemFn, StaticFunctionPtr, UnvoidStaticFunctionPtr> ClosureType;
	ClosureType m_Closure;
public:
	// Typedefs to aid generic programming
	typedef FastDelegate1 type;

	// Construction and comparison functions
	FastDelegate1() { clear(); }
	FastDelegate1(const FastDelegate1 &x) {
		m_Closure.CopyFrom(this, x.m_Closure); }
	void operator = (const FastDelegate1 &x)  {
		m_Closure.CopyFrom(this, x.m_Closure); }
	bool operator ==(const FastDelegate1 &x) const {
		return m_Closure.IsEqual(x.m_Closure);	}
	bool operator !=(const FastDelegate1 &x) const {
		return !m_Closure.IsEqual(x.m_Closure); }
	bool operator <(const FastDelegate1 &x) const {
		return m_Closure.IsLess(x.m_Closure);	}
	bool operator >(const FastDelegate1 &x) const {
		return x.m_Closure.IsLess(m_Closure);	}
	// Binding to non-const member functions
	template < class X, class Y >
	FastDelegate1(Y *pthis, DesiredRetType (X::* function_to_bind)(Param1 p1) ) {
		m_Closure.bindmemfunc(detail::implicit_cast<X*>(pthis), function_to_bind); }
	template < class X, class Y >
	inline void bind(Y *pthis, DesiredRetType (X::* function_to_bind)(Param1 p1)) {
		m_Closure.bindmemfunc(detail::implicit_cast<X*>(pthis), function_to_bind);	}
	// Binding to const member functions.
	template < class X, class Y >
	FastDelegate1(const Y *pthis, DesiredRetType (X::* function_to_bind)(Param1 p1) const) {
		m_Closure.bindconstmemfunc(detail::implicit_cast<const X*>(pthis), function_to_bind);	}
	template < class X, class Y >
	inline void bind(const Y *pthis, DesiredRetType (X::* function_to_bind)(Param1 p1) const) {
		m_Closure.bindconstmemfunc(detail::implicit_cast<const X *>(pthis), function_to_bind);	}
	// Static functions. We convert them into a member function call.
	// This constructor also provides implicit conversion
	FastDelegate1(DesiredRetType (*function_to_bind)(Param1 p1) ) {
		bind(function_to_bind);	}
	// for efficiency, prevent creation of a temporary
	void operator = (DesiredRetType (*function_to_bind)(Param1 p1) ) {
		bind(function_to_bind);	}
	inline void bind(DesiredRetType (*function_to_bind)(Param1 p1)) {
		m_Closure.bindstaticfunc(this, &FastDelegate1::InvokeStaticFunction, 
			function_to_bind); }
	// Invoke the delegate
	RetType operator() (Param1 p1) const {
	return (m_Closure.GetClosureThis()->*(m_Closure.GetClosureMemPtr()))(p1); }
	// Implicit conversion to "bool" using the safe_bool idiom
private:
	typedef struct SafeBoolStruct {
		int a_data_pointer_to_this_is_0_on_buggy_compilers;
		StaticFunctionPtr m_nonzero;
	} UselessTypedef;
    typedef StaticFunctionPtr SafeBoolStruct::*unspecified_bool_type;
public:
	operator unspecified_bool_type() const {
        return empty()? 0: &SafeBoolStruct::m_nonzero;
    }
	// necessary to allow ==0 to work despite the safe_bool idiom
	inline bool operator==(StaticFunctionPtr funcptr) {
		return m_Closure.IsEqualToStaticFuncPtr(funcptr);	}
	inline bool operator!=(StaticFunctionPtr funcptr) { 
		return !m_Closure.IsEqualToStaticFuncPtr(funcptr);    }
	inline bool operator ! () const	{	// Is it bound to anything?
			return !m_Closure; }
	inline bool empty() const	{
			return !m_Closure; }
	void clear() { m_Closure.clear();}
	// Conversion to and from the DelegateMemento storage class
	const DelegateMemento & GetMemento() { return m_Closure; }
	void SetMemento(const DelegateMemento &any) { m_Closure.CopyFrom(this, any); }

private:	// Invoker for static functions
	RetType InvokeStaticFunction(Param1 p1) const {
	return (*(m_Closure.GetStaticFunction()))(p1); }
};

class NaviData;

/*
* Functions assigned to a NaviDelegate must return a 'void' and have one argument: 'NaviData naviData'
*
* Member function instantiation example: NaviDelegate(this, &MyClass::myMemberFunction)
* Member function instantiation example: NaviDelegate(pointerToSexyClass, &SexyClass::mySexyFunction)
*
* Static function instantiation example: NaviDelegate(&myStaticFunction)
*/
typedef FastDelegate1<const NaviData&> NaviDelegate;

}

#endif

