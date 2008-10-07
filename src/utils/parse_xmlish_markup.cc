/**
 * @author George Foster
 * @file parse_xmlish_markup.cc  Parse XML-like markup.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "errors.h"
#include "parse_xmlish_markup.h"
#include "str_utils.h"

using namespace Portage;

string& XMLishTag::attrValsString(string& s)
{
   string tmp;
   for (Uint i = 0; i < attr_vals.size(); ++i)
      s += " " + attr_vals[i].first + "=\"" + XMLescape(attr_vals[i].second.c_str(), tmp) + "\"";
   return s;
}

string XMLishTag::attrVal(const string& s)
{
  string val="";
  for (vector< pair<string,string> >::const_iterator itr=attr_vals.begin(); 
       itr!=attr_vals.end(); itr++)
    if (itr->first==s) {
      val = itr->second;
      break;
    }
  return val;
}

string& XMLishTag::toString(string& s)
{
   s = "<";
   switch (tag_type) {
   case isElement:
      if (!is_beg_tag && is_end_tag) s += "/";
      s += name;
      attrValsString(s);
      if (is_beg_tag && is_end_tag) s += "/";
      break;
   case isDecl:
      s += "?" + name;
      attrValsString(s);
      s += "?";
      break;
   case isComment:
      s += "!--" + name + "--";
      break;
   case isCDATA:
      s += "![CDATA[" + name + "]]";
      break;
   case isDoctype:
      s += "!DOCTYPE " + name;
      break;
   }
   s += ">";
   return s;
}


static bool isWhite(char ch) 
{
   return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

// is char ok as 1st in name?
static bool isNameBegChar(char ch)
{
   return 
      ch >= 'a' && ch <= 'z' ||
      ch >= 'A' && ch <= 'Z' ||
      ch == ':' || ch == '_' ||
      ch & 0x80;
}

// is char ok in name?
static bool isNameChar(char ch)
{
   return
      isNameBegChar(ch) ||
      ch >= '0' && ch <= '9' ||
      ch == '.' || ch == '-';
}


static string showContext(const char buf[], const char* p)
{
   int half_context = 7;
   const char* beg = p-half_context < buf ? buf : p-half_context;
   const char* end = p;
   while (*end && end-p <= half_context) ++end;
   return "'" + string(p, 1) + "' in \"..." + string(beg, end-beg) + "...\"";
}

static const char* parseName(const char *p, const char* bufp, string& name)
{
   if (!*p) return p;
   name.clear();
   if (!isNameBegChar(*p))
      error(ETFatal, "bad initial character in XML name: " + showContext(bufp, p));
   name += *p;
   for (++p; *p && isNameChar(*p); ++p) name += *p;
   return p;
}

static const char* parseAttrVals(const char* p, const char* bufp, vector <pair<string,string> >& attr_vals)
{
   attr_vals.clear();

   while (isWhite(*p)) {

      while (isWhite(*p)) ++p;
      if (*p == '/' || *p == '>' || *p == '?' || !*p)
         break;

      attr_vals.push_back(make_pair("", ""));
      p = parseName(p, bufp, attr_vals.back().first);
      while (isWhite(*p)) ++p;

      if (*p && *p != '=')
         error(ETFatal, "expected '=' after attribute name, got: " + showContext(bufp, p));
      if (*p) ++p;
      while (isWhite(*p)) ++p;
 
      if (*p && *p != '"' && *p != '\'')
         error(ETFatal, "attribute value is unquoted: " + showContext(bufp, p));
      const char* beg = p++;
      while (*p && *p != *beg) ++p;
      attr_vals.back().second = string(beg+1, p-(beg+1));

      if (*p) ++p;
   }
   
   return p;
}

static bool parseEnd(const char* p, const char* bufp, Uint* end, const char* err_msg)
{
   if (*p == '>') {
      if (end) *end = p+1-bufp;
      return true;
   } else if (*p)
      error(ETFatal, err_msg + showContext(bufp, p));
   return false;
}

bool Portage::parseXMLishTag(const char buf[], XMLishTag& tag, Uint *beg, Uint* end)
{
   // find initial < or eob
   const char* p;
   for (p = buf; *p && *p != '<'; ++p) ;

   if (beg) *beg = Uint(p-buf);

   // parse various types of tags depending on 1st character
   if (*p) ++p;
   if (*p == '?') {		// <?...?>
      tag.tag_type = XMLishTag::isDecl;
      p = parseName(++p, buf, tag.name);
      p = parseAttrVals(p, buf, tag.attr_vals);
      if (*p && *p != '?')
         error(ETFatal, "expected '?' at end of declaration, got: " + showContext(buf, p));
      if (*p) ++p;
      return parseEnd(p, buf, end, "bad character after declaration: ");
   } else if (*p == '!') {	// <!--...--> or <![CDATA[...]]> or <!DOCTYPE ...>
      ++p;
      if (isPrefix("--", p)) {
         tag.tag_type = XMLishTag::isComment;
         const char* b = p + 2;
         for (p = b; *p; ++p)
            if (isPrefix("--", p)) break;
         if (*p) {
            tag.name = string(b, p-b);
            p += 2;
         }
         return parseEnd(p, buf, end, "bad character after comment: ");
      } else if (isPrefix("DOCTYPE", p)) {
         tag.tag_type = XMLishTag::isDoctype;
         const char* b = p + strlen("DOCTYPE");
         for (p = b; *p; ++p)
           if (isPrefix(">", p)) {
             tag.name = string(b, p-b);
             if (end) *end = p+strlen(">")-buf;
             return true;
           }
         return false;
      } else if (isPrefix("[CDATA[", p)) {
         tag.tag_type = XMLishTag::isCDATA;
         const char* b = p + strlen("[CDATA[");
         for (p = b; *p; ++p)
            if (isPrefix("]]>", p)) {
               tag.name = string(b, p-b);
               if (end) *end = p+strlen("]]>")-buf;
               return true;
            }
         return false;
      } else if (*p) {
         error(ETFatal, "expected '--' or '[CDATA[' or 'DOCTYPE' after '!', got: " + showContext(buf, p));
      } else
         return false;
   } else if (*p == '/') {	// </...>
      tag.tag_type = XMLishTag::isElement;
      tag.is_beg_tag = false;
      tag.is_end_tag = true;
      p = parseName(++p, buf, tag.name);
      while (isWhite(*p)) ++p;
      return parseEnd(p, buf, end, "bad character after XML name: ");
   } else {			// <...> or <.../>
      tag.tag_type = XMLishTag::isElement;
      tag.is_beg_tag = true;
      tag.is_end_tag = false;
      p = parseName(p, buf, tag.name);
      p = parseAttrVals(p, buf, tag.attr_vals);
      if (*p == '/') {
         tag.is_end_tag = true;
         ++p;
      }
      return parseEnd(p, buf, end, "bad character after XML name: ");
   }
   assert(0);
}

static const char xml_specials[] = {'<', '>', '&', '\'', '"'};
static const char* xml_escapes[]  = {"lt", "gt", "amp", "apos", "quot"};

string& Portage::XMLescape(const char buf[], string& dest, Uint buflen)
{
   dest.clear();
   for (const char* p = buf; *p && (buflen == 0 || p-buf < int(buflen)); ++p) {
      Uint i = 0;
      for (i = 0; i < ARRAY_SIZE(xml_specials); ++i)
         if (*p == xml_specials[i]) {
            dest += '&' + string(xml_escapes[i]) + ';';
            break;
         }
      if (i == ARRAY_SIZE(xml_specials))
         dest += *p;
   }
   return dest;
}

string& Portage::XMLunescape(const char buf[], string& dest, Uint buflen)
{
   dest.clear();
   for (const char* p = buf; *p && (buflen == 0 || p-buf < int(buflen)); ++p) {
      if (*p == '&') {
         Uint i;
         for (i = 0; i < ARRAY_SIZE(xml_escapes); ++i)
            if (isPrefix(xml_escapes[i], p+1) && *(p+1+strlen(xml_escapes[i])) == ';')
               break;
         if (i < ARRAY_SIZE(xml_escapes)) {
            dest += xml_specials[i];
            p += strlen(xml_escapes[i]) + 1;
            continue;
         }
      }
      dest += *p;
   }
   return dest;
}
