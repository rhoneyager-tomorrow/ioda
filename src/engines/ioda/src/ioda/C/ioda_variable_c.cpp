/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_variable_c.hpp"


extern "C" {

void * ioda_variable_c_alloc()
{
    return reinterpret_cast<void*>(new ioda::Variable());
}

void ioda_variable_c_dtor(void **p) {
    void *p_ = *p;
    VOID_TO_CXX(ioda::Variable,p_,v);
    if ( v != nullptr ) delete v;
    v = nullptr;
    *p = nullptr;
}

void ioda_variable_c_clone(void **t_p,void *rhs_p)
{
    try {
        ioda::Variable ** t = 
            reinterpret_cast< ioda::Variable ** >(t_p);
        VOID_TO_CXX(ioda::Variable,rhs_p,rhs);
        if ( *t != nullptr) {
            delete *t;    
        }
        if ( rhs == nullptr) {
            return;
        }
        *t = new ioda::Variable(*rhs);
        t_p = reinterpret_cast< void ** >(t);
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_variable_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

void * ioda_variable_c_has_attributes(void *p) {
    try {
       VOID_TO_CXX(ioda::Variable,p,var);
       if (var==nullptr) {
            std::cerr << "ioda_variable_c_has_attributes variable ptr is null\n";
            fatal_error();
       }
       return reinterpret_cast<void*>(new ioda::Has_Attributes(var->atts));
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_has_attributes_failed\n";
        fatal_error();
    }
    return nullptr;
}

void * ioda_variable_c_get_dimensions(void *p) {
    try{
        VOID_TO_CXX(ioda::Variable,p,var);
        if (var==nullptr) {
            std::cerr << "ioda_variable_c_get_dimensions variable ptr is null\n";
            fatal_error();
        }
        ioda::Dimensions * dims = new ioda::Dimensions(var->getDimensions());
        return reinterpret_cast<void*>(dims);
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_get_dimensions failed\n";
        fatal_error();
    }
    return nullptr;
}

bool ioda_variable_c_resize(void *p,int64_t n,void *dim_ptr) {
    try{
        VOID_TO_CXX(ioda::Variable,p,var);
        VOID_TO_CXX(int64_t,dim_ptr,dims);
        if (var==nullptr) {
            std::cerr << "ioda_variable_c_resize variable ptr is null\n";
            fatal_error();
        }
        if ( dims == nullptr) {
            std::cerr << "ioda_variable_c_resize dimensions ptr is null\n";
            fatal_error();
        }
        ioda::Variable vret = var->resize(std::vector<ptrdiff_t>(dims,dims+n));
        return true;
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_resizes failed\n";
    }
    return false;
}

bool ioda_variable_c_attach_dim_scale(void *p,int32_t dim_n,void *var_ptr) {
    try{
        VOID_TO_CXX(ioda::Variable,p,var);
        VOID_TO_CXX(ioda::Variable,var_ptr,vp);
        if (var==nullptr) {
            std::cerr << "ioda_variable_attach_dims_scale variable ptr is null\n";
            fatal_error();
        }
        if ( vp == nullptr) {
            std::cerr << "ioda_variable_c_attach_dim_scale second variable ptr is null\n";
            fatal_error();
        }
        ioda::Variable vr = var->attachDimensionScale(dim_n,*vp);
        return true;
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_attach_dim_scale failed\n";
    }
    return false;
}

bool ioda_variable_c_detach_dim_scale(void *p,int32_t dim_n,void *var_ptr) {
    try{
        VOID_TO_CXX(ioda::Variable,p,var);
        VOID_TO_CXX(ioda::Variable,var_ptr,vp);

        if (var==nullptr) {
            std::cerr << "ioda_variable_c_detach_dim_scale ptr is null\n";
            fatal_error();
        }
        if ( vp == nullptr) {
            std::cerr << "ioda_variable_c_detrach_dim_scale second variable ptr is null\n";
            fatal_error();
        }
        ioda::Variable vr = var->detachDimensionScale(dim_n,*(vp));
        return true;
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_detach_dims_scale failed\n";
    }
    return false;
}

bool ioda_variable_c_set_dim_scale(void *p,int64_t ndim,void **var_ptr) {
    try{
        VOID_TO_CXX(ioda::Variable,p,var);
        VOID_TO_CXX(ioda::Variable*,var_ptr,vp);
        if (var==nullptr) {
            std::cerr << "ioda_variable_c_set_dim_scale ptr is null\n";
            fatal_error();
        }
        if ( vp == nullptr) {
            std::cerr << "ioda_variable_c_detrach_dim_scale second variable ptr is null\n";
            fatal_error();
        }
        std::vector< ioda::Variable > newDims(*vp,(*vp)+ndim);
        ioda::Variable vr = var->setDimScale(newDims);
        return true;
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_set_dim_scales failed\n";
    }
    return false;
}

int32_t ioda_variable_c_is_dim_scale(void *p) {
    try{
        VOID_TO_CXX(ioda::Variable,p,var);
        if (var==nullptr) {
            std::cerr << "ioda_variable_c_is_dim_scale ptr is null\n";
            fatal_error();
        }         
        return (var->isDimensionScale()) ? 1:0;
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_is_dims_scale failed\n";
    }
    return -1;
}

bool ioda_variable_c_set_is_dimension_scale(void *p,int64_t sz,void *name_p) {
    try{
        VOID_TO_CXX(ioda::Variable,p,var);
        VOID_TO_CXX(const char,name_p,name);
        if (var==nullptr) {
            std::cerr << "ioda_variable_c_set_is_dim_scale ptr is null\n";
            fatal_error();
        }
        if ( name == nullptr && sz!=0) {
            std::cerr << "ioda_variable_c_set_is_dim_scale name string  is null\n";
            fatal_error();
        }
        ioda::Variable vr = var->setIsDimensionScale(std::string(name,sz));
        return true;
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_set_is_dim_scale failed\n";
    }
    return false;
}

int64_t ioda_variable_c_get_dimension_scale_name(void *p,int64_t n,void **out_p) {
    try{
        VOID_TO_CXX(ioda::Variable,p,var);
        void * out_p_ = *out_p;
        VOID_TO_CXX(char,out_p_,name_p);
        if (var==nullptr) {
            std::cerr << "ioda_variable_c_get_dimension_scale_name _scale ptr is null\n";
            fatal_error();
        }
        std::string r;
        ioda::Variable vr = var->getDimensionScaleName(r);
        if ( r.size() == std::string::npos) {
           std::cerr << "ioda_variable_c_get_dimension_scale_name name is too long\n";
           throw std::exception();
        }
        int64_t rsz = r.size() + 1;
        if ( name_p == 0x0) {
             name_p = reinterpret_cast<char*>(Malloc(rsz));
        }else{
             if ( rsz > (n+1) ) {
                free(name_p);
                name_p = reinterpret_cast<char*>(Malloc(rsz));
             } 
        }
        strncpy(name_p,r.c_str(),n);
        *out_p = reinterpret_cast<void*>(name_p);
        return rsz;
    } catch (std::exception& e) {
    }
    return 0;
}

int32_t ioda_variable_c_is_dimension_scale_attached(void *p,int32_t dim_num,void *scale_p) {
    try{
        VOID_TO_CXX(ioda::Variable,p,var);
        VOID_TO_CXX(ioda::Variable,scale_p,scale);
        if (var==nullptr) {
            std::cerr << "ioda_variable_c_is_dimension_scale_attached maine ptr  is null\n";
            fatal_error();
        }
        if (scale==nullptr) {
            std::cerr << "ioda_variable_c_is_dimension_scale_attached maine ptr  is null\n";
            fatal_error();
        }
        auto b = var->isDimensionScaleAttached(dim_num,*(scale));
        return (b) ? 1:0;
    } catch (std::exception& e) {
    }
    return -1;
}

#define IODA_FUN(NAME,TYPE) \
int32_t ioda_variable_c_is_a##NAME (void * p) {				\
    try{								\
        VOID_TO_CXX(ioda::Variable,p,var);				\
        if (var==nullptr) {						\
            std::cerr << "ioda_variable_c_is_a  ptr is null\n";		\
            fatal_error();						\
        }								\
        bool b = var->isA< TYPE >();					\
        return (b)?1:0;							\
    } catch (std::exception& e) {					\
        std::cerr << "ioda_variable_c_is_a failed\n";			\
    } 									\
    return -1;								\
}

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
    
#define IODA_FUN(NAME,TYPE)\
bool ioda_variable_c_write##NAME(void *p,int64_t n, const TYPE *data) {		\
    try {									\
       VOID_TO_CXX(ioda::Variable,p,var);					\
       if ( var == nullptr ) {							\
           std::cerr << "ioda_variable_c_write var pointer is null\n"; 		\
           fatal_error();							\
       } 									\
       ioda::Variable v = var->write< TYPE >(gsl::span< const TYPE >(data,n));	\
       return true;								\
    } catch (std::exception& e) {						\
        std::cerr << "ioda_variable_c_has_attributes_write\n"; 			\
        return false;								\
    }										\
}

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN


bool ioda_variable_c_write_char(void *p,int64_t n,void * vptr) {
    try {
        VOID_TO_CXX(ioda::Variable,p,var);
        if ( var == nullptr) {
           std::cerr << "ioda_variable_write_char variable pointer is null\n";
           throw std::exception();    
        }
        VOID_TO_CXX(const char,vptr,str);
        if ( str == nullptr) {
             std::cerr << "ioda_variable_write_char char array is null\n";
             throw std::exception();      
        }
        std::string s(str,n);
        auto v = var->write< std::string >(s);  
        return true;
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_write_char failed\n";
        std::cerr << e.what() << "\n"; 
    }
    return false;
}

bool ioda_variable_c_write_str(void *p,void *vstr_p) {
    try {				
       VOID_TO_CXX(ioda::Variable,p,var);
       VOID_TO_CXX(std::vector<std::string>,vstr_p,vstr);
       if ( var == nullptr ) {
           std::cerr << "ioda_variable_c_write_str var pointer is null\n";
           fatal_error();					
       } 							
       if ( vstr == nullptr ) {
           std::cerr << "ioda_variable_c_write_str vecstring pointer is null\n";
           fatal_error();					
       } 			
       ioda::Variable v = var->write< std::vector< std::string > >(*vstr);
       return true;						
    } catch (std::exception& e) {				
        std::cerr << "ioda_variable_c_write_str_full failed ";
        std::cerr << e.what() << "\n"; 	
    }								
    return false;
}

#define IODA_FUN(NAME,TYPE)\
bool ioda_variable_c_read##NAME(void *p,int64_t n,void **dptr) {\
    try {										\
       VOID_TO_CXX(ioda::Variable,p,var);						\
       if (var == nullptr) {								\
           std::cerr << "ioda_variable_c_read variable pointer is null\n";		\
           fatal_error();								\
       } 										\
       void * v_dp;									\
       if ( dptr != nullptr) {								\
          v_dp = *dptr;									\
       }else{										\
          v_dp = nullptr; 								\
       }										\
       if ( v_dp == nullptr) {								\
          v_dp = Malloc(sizeof( TYPE )*n);						\
       }  										\
       VOID_TO_CXX( TYPE ,v_dp,data);							\
       auto vr = var->read< TYPE >(gsl::span< TYPE >(data,n));				\
       /* make sure that vstr points to correct data pointer */				\
       *dptr = reinterpret_cast<void*>(data); 						\
       return true;									\
    } catch (std::exception& e) {							\
        std::cerr << "ioda_variable_c_read failed ";					\
        std::cerr << e.what() << "\n";    						\
    } 											\
    return false;  									\
}


IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN

bool ioda_variable_c_read_char(void *p,int64_t n,void **vstr) {
    try {
       VOID_TO_CXX(ioda::Variable,p,var);
       if (var == nullptr) {
           std::cerr << "ioda_variable_c_read_char variable pointer is null\n";
           fatal_error();
       } 
       void * vs_p;
       if ( vstr != nullptr) {
          vs_p = *vstr;
       }else{
          vs_p = nullptr; 
       }
       if ( vs_p == nullptr) {
          vs_p = Malloc(n+1); 
       }  
       VOID_TO_CXX(char,vs_p,str);
       auto vr = var->read<char>(gsl::span<char>(str,n));
       // make sure that vstr points to correct char *
       *vstr = reinterpret_cast<void*>(str);
       return true;
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_read_char failed ";
        std::cerr << e.what() << "\n";    
    }
    return false;  
}

bool ioda_variable_c_read_str(void *p,int64_t n,void **vstr) {
    try {
       VOID_TO_CXX(ioda::Variable,p,var);
       if (var == nullptr) {
           std::cerr << "ioda_variable_c_read_str variable pointer is null\n";
           fatal_error();
       } 
       void * vs_p;
       if ( vstr != nullptr) {
          vs_p = *vstr;
       } else {
          vs_p = reinterpret_cast<void*>(new std::vector<std::string>()); 
       }  
       VOID_TO_CXX(std::vector<std::string>,vs_p,vs);
       auto vr = var->read< std::vector<std::string> >(*vs);
       // make sure that vstr points to correct char *
       *vstr = reinterpret_cast<void*>(vs);
       return true;
    } catch (std::exception& e) {
        std::cerr << "ioda_variable_c_read_char failed ";
        std::cerr << e.what() << "\n";    
    }
    return false;  
}

}




