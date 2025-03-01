/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_variable_creation_parameters_c.hpp"

extern "C" {

void * ioda_variable_creation_parameters_c_alloc()
{
        return reinterpret_cast<void*>( new ioda::VariableCreationParameters() );
}

void ioda_variable_creation_parameters_c_dtor(void **p) {
        if (p==nullptr || *p == nullptr) return;
        VOID_TO_CXX(ioda::VariableCreationParameters,*p,c_params);
        if ( c_params ) {
            delete c_params;
        }
        *p = nullptr;
}

void ioda_variable_creation_parameters_c_clone(void **t_p,void *rhs_p)
{
    try {
        ioda::VariableCreationParameters ** t = 
            reinterpret_cast< ioda::VariableCreationParameters ** >(t_p);
        VOID_TO_CXX(ioda::VariableCreationParameters,rhs_p,rhs);
        if ( *t != nullptr) {
            delete *t;    
        }
        if ( rhs == nullptr) {
            return;
        }
        *t = new ioda::VariableCreationParameters(*rhs);
        t_p = reinterpret_cast< void ** >(t);
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_variable_creation_parameters_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

void ioda_variable_creation_parameters_c_chunking(void *p,bool do_chunking,int64_t ndims,const ptrdiff_t *chunks) {
    try {
        VOID_TO_CXX(ioda::VariableCreationParameters,p,c_params);
        if ( c_params == nullptr || chunks==nullptr) {
            std::cerr << "ioda_variable_creation_parameters_c_chunking nullptr\n";
            throw std::exception();
        }
        c_params->chunk = do_chunking;
        if ( do_chunking ) {
            (c_params->chunks) = std::vector<ptrdiff_t>(chunks,chunks+ndims);
        }
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_creation_parameters_c_chunking failed\n";
        fatal_error();        
    }
}

void ioda_variable_creation_parameters_c_no_compress(void *p) {
    try {
        VOID_TO_CXX(ioda::VariableCreationParameters,p,c_params);
        if ( c_params == nullptr ) {
            std::cerr << "ioda_variable_creation_parameters_c_nocompress nullptr\n";
            throw std::exception();
        }
        c_params->noCompress();
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_creation_parameters_c_no_compress failed\n";
        fatal_error();        
    }
}

void ioda_variable_creation_parameters_c_compress_with_gzip(void *p,int32_t level) {
    try {
        VOID_TO_CXX(ioda::VariableCreationParameters,p,c_params);
        if ( c_params == nullptr ) {
            std::cerr << "ioda_variable_creation_parameters_c_ompress_with_gzip nullptr\n";
            throw std::exception();
        }
        c_params->compressWithGZIP(level);
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_creation_parameters_c_compress_with_gzi[ failed\n";
        fatal_error();        
    }
}

void ioda_variable_creation_parameters_c_compress_with_szip(void *p,int32_t pixels_per_block,int32_t options) {
    try {
        VOID_TO_CXX(ioda::VariableCreationParameters,p,c_params);
        if ( c_params == nullptr ) {
            std::cerr << "ioda_variable_creation_parameters_c_ompress_with_szip nullptr\n";
            throw std::exception();
        }
        c_params->compressWithSZIP(static_cast<uint32_t>(pixels_per_block),
                static_cast<uint32_t>(options));
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_creation_parameters_c_compress_with_szip failed\n";
        fatal_error();        
    }
}

#define IODA_FUN(NAME,TYPE) \
void ioda_variable_creation_parameters_c_set_fill_value##NAME(void *p, TYPE value) {		\
    try {											\
        VOID_TO_CXX(ioda::VariableCreationParameters,p,c_params);				\
        if ( c_params == nullptr ) {								\
            std::cerr << "ioda_variable_creation_parameters_c_set_fill_value nullptr\n";	\
            throw std::exception();								\
        }											\
        c_params->setFillValue< TYPE >(value);							\
    } catch (std::exception& e) {								\
        std::cerr << "ioda_variable_creation_parameters_c_set_fill_value failed\n";		\
        fatal_error();       									\
    } 												\
}

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN    

}




