/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_attribute_c.hpp"
#include <gsl/gsl-lite.hpp>

extern "C" {

void * ioda_attribute_c_alloc()
{
     return reinterpret_cast<void*>(new ioda::Attribute());
}

void ioda_attribute_c_dtor(void **v) {
    VOID_TO_CXX(ioda::Attribute*,v,p);
    if ( *p == nullptr ) {
        delete *p;
    }
    *p = nullptr;
    *v = nullptr;
}

bool ioda_attribute_c_is_allocated(void *v)
{
    VOID_TO_CXX(ioda::Attribute,v,p);
    return (p==nullptr) ? false:true; 
}

void ioda_attribute_c_clone(void **t_p,void *rhs_p)
{
    try {
        ioda::Attribute ** t = 
            reinterpret_cast< ioda::Attribute ** >(t_p);
        VOID_TO_CXX(ioda::Attribute,rhs_p,rhs);
        if ( *t != nullptr) {
            delete *t;    
        }
        if ( rhs == nullptr) {
            return;
        }
        *t = new ioda::Attribute(*rhs);
        t_p = reinterpret_cast< void ** >(t);
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_attribute_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

void * ioda_attribute_c_get_dimensions(void *v)  
{
    try {
        VOID_TO_CXX(ioda::Attribute,v,p);
        if ( p == nullptr) {
            std::cerr << "ioda_attribute_c_get_dimensions null pointer in arguments\n";
            throw std::exception();
        }
        return reinterpret_cast<void*>( new ioda::Dimensions( p->getDimensions() ) );
    } catch (std::exception& e) {
        std::cerr << "ioda_attribute_c_get_dimensions failed\n" << e.what() << "\n";
        fatal_error(); 
    }
    return 0x0;
}

#define IODA_FUN(NAME,TYPE)\
bool ioda_attribute_c_write##NAME (void *v,int64_t n, const TYPE * data) {		\
    try  {										\
        VOID_TO_CXX(ioda::Attribute,v,p);						\
        ioda::Attribute ret = p->write< TYPE >(gsl::span< const TYPE >(data,n));	\
        return true;									\
    } catch (std::exception& e) {							\
    } 											\
    return false;									\
}    											\

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN

bool ioda_attribute_c_write_char (void *v,int64_t n, const void * data) {
    try  { 
        VOID_TO_CXX(ioda::Attribute,v,p);
        VOID_TO_CXX(const char,data,data_p);
        ioda::Attribute ret = p->write< char >(gsl::span< const char >(data_p,n));
        return true;							
    } catch (std::exception& e) {					
    } 									
    return false;							
}    									


bool ioda_attribute_c_write_str(void *v,void *vvs) {
    try { 
        VOID_TO_CXX(ioda::Attribute,v,p);
        VOID_TO_CXX(std::vector<std::string>,vvs,vs);
        ioda::Attribute ax = p->write< std::vector<std::string> >(*vs);
        return true;
    } catch (std::exception& e) {}
    return false;
}


#define IODA_FUN(NAME,TYPE)\
bool ioda_attribute_c_read##NAME (void *v,int64_t n, TYPE * data) {			\
    try  {										\
        VOID_TO_CXX(ioda::Attribute,v,p);						\
        ioda::Attribute ret = p->read< TYPE >(gsl::span< TYPE >(data,n));		\
        return true;									\
    } catch (std::exception& e) {							\
    }											\
    return false;									\
}    											\

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN

bool ioda_attribute_c_read_char (void *v,int64_t n, void * data) {
    try  { 
        VOID_TO_CXX(ioda::Attribute,v,p);
        VOID_TO_CXX(char,data,data_p);
        ioda::Attribute ret = p->read< char >(gsl::span< char >(data_p,n));
        return true;							
    } catch (std::exception& e) {					
    } 									
    return false;							
}    									

bool  ioda_attribute_c_read_str(void *v,void * vvs) {
    try { 
        VOID_TO_CXX(ioda::Attribute,v,p);
        VOID_TO_CXX(std::vector<std::string>,vvs,vs);
        ioda::Attribute ax = p->read< std::vector<std::string> >(*vs);
        return true;
    } catch (std::exception& e) {
    }
    return false;
}

#define IODA_FUN(NAME,TYPE)\
int ioda_attribute_c_is_a##NAME (void *v) { 					\
    try  {									\
        VOID_TO_CXX(ioda::Attribute,v,p);					\
        bool b = p->isA< TYPE >();						\
        return b ? 1:0;								\
    } catch (std::exception& e) {}						\
    return -1;									\
}    										\

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_ldouble,long double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
IODA_FUN(_uint16,uint16_t)
IODA_FUN(_uint32,uint32_t)
IODA_FUN(_uint64,uint64_t)
IODA_FUN(_str,std::vector<std::string>)
#undef IODA_FUN
}