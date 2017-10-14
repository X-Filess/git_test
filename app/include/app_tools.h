#ifndef __APP_TOOLS_H__
#define __APP_TOOLS_H__

//added by yingc@nationalchip.com
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unistd.h>
#include <algorithm>
#include <cassert>

#include "gxcore.h"
 

using namespace std;
//__BEGIN_DECLS

template < class T > 
void ClearVector( std::vector< T >& vt ) 
{
    std::vector< T > vtTemp; 
    vtTemp.swap( vt );
}

template<class T>
inline T FromString(const string &str)
{
    istringstream is(str);
    T v;
    is>>v;
    return v;
}

template<class T>
inline string ToString(const T &v)
{
    ostringstream os;
    os<<v;
    return os.str();
}

#ifndef foreach  
#define foreach(container,it) \
for(typeof((container).begin()) it = (container).begin();it!=(container).end();++it)  
#endif  


void StringReplace(string & sSrc, const string &sOld, const string &sNew);
std::string UrlEncode(const std::string& str);
std::string UrlDecode(const std::string& str);
std::string& StringTrim(std::string &s);

//__END_DECLS

#endif