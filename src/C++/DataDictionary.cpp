/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#include "DataDictionary.h"
#include "Message.h"
#include <fstream>
#include <memory>

#include "PUGIXML_DOMDocument.h"

#ifdef _MSC_VER
#define RESET_AUTO_PTR(OLD, NEW) OLD = NEW;
#else
#define RESET_AUTO_PTR(OLD, NEW) OLD.reset( NEW.release() );
#endif

namespace FIX
{
DataDictionary::DataDictionary()
: m_hasVersion( false ), m_checks(AllChecks),
  m_messageFields(get_allocator<MsgTypeToField>()),
  m_requiredFields(get_allocator<MsgTypeToField>()),
  m_messages(get_allocator<MsgTypes>()),
  m_fields(get_allocator<Fields>()),
  m_orderedFields(get_allocator<OrderedFields>()),
  m_headerFields(NonBodyFields::key_compare(), get_allocator<NonBodyFields>()),
  m_trailerFields(NonBodyFields::key_compare(), get_allocator<NonBodyFields>()),
  m_fieldTypes(get_allocator<FieldTypes>()),
  m_fieldValues(get_allocator<FieldToValue>()),
  m_groups(get_allocator<FieldToGroup>())
{}

DataDictionary::DataDictionary( std::istream& stream )
throw( ConfigError )
: m_hasVersion( false ), m_checks(AllChecks),
  m_messageFields(get_allocator<MsgTypeToField>()),
  m_requiredFields(get_allocator<MsgTypeToField>()),
  m_messages(get_allocator<MsgTypes>()),
  m_fields(get_allocator<Fields>()),
  m_orderedFields(get_allocator<OrderedFields>()),
  m_headerFields(NonBodyFields::key_compare(), get_allocator<NonBodyFields>()),
  m_trailerFields(NonBodyFields::key_compare(), get_allocator<NonBodyFields>()),
  m_fieldTypes(get_allocator<FieldTypes>()),
  m_fieldValues(get_allocator<FieldToValue>()),
  m_groups(get_allocator<FieldToGroup>())
{
  readFromStream( stream );
}

DataDictionary::DataDictionary( const std::string& url )
throw( ConfigError )
: m_hasVersion( false ), m_checks(AllChecks),
  m_messageFields(get_allocator<MsgTypeToField>()),
  m_requiredFields(get_allocator<MsgTypeToField>()),
  m_messages(get_allocator<MsgTypes>()),
  m_fields(get_allocator<Fields>()),
  m_orderedFields(get_allocator<OrderedFields>()),
  m_headerFields(NonBodyFields::key_compare(), get_allocator<NonBodyFields>()),
  m_trailerFields(NonBodyFields::key_compare(), get_allocator<NonBodyFields>()),
  m_fieldTypes(get_allocator<FieldTypes>()),
  m_fieldValues(get_allocator<FieldToValue>()),
  m_groups(get_allocator<FieldToGroup>())
{
  readFromURL( url );
}

DataDictionary::DataDictionary( const DataDictionary& copy )
: DataDictionaryBase(copy),
  m_hasVersion( false ), m_checks(AllChecks),
  m_messageFields(get_allocator<MsgTypeToField>()),
  m_requiredFields(get_allocator<MsgTypeToField>()),
  m_messages(get_allocator<MsgTypes>()),
  m_fields(get_allocator<Fields>()),
  m_orderedFields(get_allocator<OrderedFields>()),
  m_headerFields(NonBodyFields::key_compare(), get_allocator<NonBodyFields>()),
  m_trailerFields(NonBodyFields::key_compare(), get_allocator<NonBodyFields>()),
  m_fieldTypes(get_allocator<FieldTypes>()),
  m_fieldValues(get_allocator<FieldToValue>()),
  m_groups(get_allocator<FieldToGroup>())
{
  *this = copy;
}

DataDictionary::~DataDictionary()
{
  for ( FieldToGroup::iterator i = m_groups.begin(); i != m_groups.end(); ++i )
  {
    const FieldPresenceMap& presenceMap = i->second;

    FieldPresenceMap::const_iterator iter = presenceMap.begin();
    for ( ; iter != presenceMap.end(); ++iter )
      delete iter->second.second;
  }
}

DataDictionary& DataDictionary::operator=( const DataDictionary& rhs )
{
  m_hasVersion = rhs.m_hasVersion;
  m_checks = rhs.m_checks;
  m_beginString = rhs.m_beginString;

  for (MsgTypeToField::const_iterator i = rhs.m_messageFields.begin();
       i != rhs.m_messageFields.end(); ++i)
  {
    MsgTypeToField::iterator it = m_messageFields.insert(
     MsgTypeToField::value_type( i->first,
      MsgFields(MsgFields::key_compare(), get_allocator<MsgFields>() ) ) ).first;
    it->second = i->second;
  }

  for (MsgTypeToField::const_iterator i = rhs.m_requiredFields.begin();
       i != rhs.m_requiredFields.end(); ++i)
  {
    MsgTypeToField::iterator it = m_requiredFields.insert(
     MsgTypeToField::value_type( i->first,
      MsgFields( MsgFields::key_compare(), get_allocator<MsgFields>() ) ) ).first;
    it->second = i->second;
  }

  for (FieldToValue::const_iterator i = rhs.m_fieldValues.begin();
       i != rhs.m_fieldValues.end(); ++i)
  {
    FieldToValue::iterator it = m_fieldValues.insert(
      FieldToValue::value_type( i->first,
        Values( get_allocator<Values>() ) ) ).first;
    it->second = i->second;
  }

  m_fieldTypeGroup = rhs.m_fieldTypeGroup;
  for (FieldToGroup::const_iterator i = rhs.m_groups.begin();
       i != rhs.m_groups.end(); ++i )
  {
    const FieldPresenceMap& presenceMap = i->second;

    FieldPresenceMap::const_iterator iter = presenceMap.begin();
    for ( ; iter != presenceMap.end(); ++iter )
    {
      addGroup( iter->first, i->first, iter->second.first, *iter->second.second );
    }
  }

  m_messages = rhs.m_messages;
  m_fields = rhs.m_fields;
  m_orderedFields = rhs.m_orderedFields;
  m_orderedFieldsArray = rhs.m_orderedFieldsArray;
  m_headerFields = rhs.m_headerFields;
  m_trailerFields = rhs.m_trailerFields;
  m_fieldTypes = rhs.m_fieldTypes;
  m_fieldTypeData = rhs.m_fieldTypeData;
  m_fieldTypeHeader = rhs.m_fieldTypeHeader;
  m_fieldTypeTrailer = rhs.m_fieldTypeTrailer;
  m_fieldNames = rhs.m_fieldNames;
  m_names = rhs.m_names;
  m_valueNames = rhs.m_valueNames;

  return *this;
}


void HEAVYUSE DataDictionary::validate( const Message& message, bool bodyOnly ) const
throw( FIX::Exception )
{
  const Header& header = message.getHeader();
  FieldMap::iterator it = header.begin();
  FieldMap::iterator end = header.end();
  if( LIKELY(it != end && it->first == FIELD::BeginString) )
  {
    const BeginString& beginString = (const BeginString&) it->second;
    if( LIKELY(++it != end) )
    {
      if( LIKELY(it->first == FIELD::MsgType) )
      {
        validate( message, beginString, (const MsgType&) it->second,
              bodyOnly ? (DataDictionary*)NULL : this, this );
        return;
      }
      if( LIKELY(++it != end && it->first == FIELD::MsgType) )
      {
        validate( message, beginString, (const MsgType&) it->second,
              bodyOnly ? (DataDictionary*)NULL : this, this );
        return;
      }
    }
    throw FieldNotFound( FIELD::MsgType );
  }
  throw FieldNotFound( FIELD::BeginString );
}

void HEAVYUSE DataDictionary::validate( const Message& message,
                               const BeginString& beginString,
                               const MsgType& msgType,
                               const DataDictionary* const pSessionDD,
                               const DataDictionary* const pAppDD )
throw( FIX::Exception )
{
  unsigned session_checks, app_checks;

  if( LIKELY(pSessionDD != 0) )
  {
    if( pSessionDD->m_hasVersion )
    {
      if( pSessionDD->getVersion() != beginString )
      {
        throw UnsupportedVersion();
      }
    }
    session_checks = pSessionDD->m_checks;
  }
  else
    session_checks = 0;

  app_checks = pAppDD ? pAppDD->m_checks : 0;
  session_checks |= app_checks;

  const Header& header = message.getHeader();

  if( session_checks ) 
  {
    const char* tag = message.hasInvalidTagFormat();
    int field = 0;

    if( tag )
      throw InvalidTagNumber(tag, ::strchr(tag, '=') - tag);

    if( isChecked(FieldsOutOfOrder, session_checks) )
    {
      if ( !message.hasValidStructure(field) )
        throw TagOutOfOrder(field);
    }
  
    const Trailer& trailer = message.getTrailer();

    if ( pAppDD != 0 && pAppDD->m_hasVersion )
    {
      if( isChecked(UnknownMsgType, app_checks) )
        pAppDD->checkMsgType( msgType );
      if( isChecked(RequiredFields, app_checks) )
        pAppDD->checkHasRequired( header, message, trailer, msgType );
    }
  
    if( pSessionDD != 0 )
    {
      pSessionDD->iterate( header, msgType );
      pSessionDD->iterate( trailer, msgType );
    }
  
    if( pAppDD != 0 )
      pAppDD->iterate( message, msgType );
  }
}

void HEAVYUSE DataDictionary::iterate( const FieldMap& map, const MsgType& msgType ) const
{
  int hasBegin = m_beginString.length();
  int lastField = 0;

  FieldMap::iterator i, ibegin = map.begin(), iend = map.end();
  for ( i = ibegin; i != iend; ++i )
  {
    const FieldBase& field = i->second;
    if( LIKELY((field.getTag() != lastField) || i == ibegin) )
    {
      checkHasValue( field );
  
      if ( m_hasVersion )
      {
        checkValidFormat( field );
        checkValue( field );
      }
  
      if ( hasBegin && shouldCheckTag(field) )
      {
        checkValidTagNumber( field );
        if ( !Message::isHeaderField( field, this )
             && !Message::isTrailerField( field, this ) )
        {
          checkIsInMessage( field, msgType );
          checkGroupCount( field, map, msgType );
        }
      }
      lastField = field.getTag();
      continue;
    }
    throw RepeatedTag( lastField );
  }
}

void DataDictionary::readFromURL( const std::string& url )
throw( ConfigError )
{
  DOMDocumentPtr pDoc = DOMDocumentPtr(new PUGIXML_DOMDocument());

  if(!pDoc->load(url))
    throw ConfigError(url + ": Could not parse data dictionary file");

  try
  {
    readFromDocument( pDoc );
  }
  catch( ConfigError& e )
  {
    throw ConfigError( url + ": " + e.what() );
  }
}

void DataDictionary::readFromStream( std::istream& stream )
throw( ConfigError )
{
  DOMDocumentPtr pDoc = DOMDocumentPtr(new PUGIXML_DOMDocument());

  if(!pDoc->load(stream))
    throw ConfigError("Could not parse data dictionary stream");

  readFromDocument( pDoc );
}

void DataDictionary::readFromDocument( DOMDocumentPtr pDoc )
throw( ConfigError )
{
  // VERSION
  DOMNodePtr pFixNode = pDoc->getNode("/fix");
  if(!pFixNode.get())
    throw ConfigError("Could not parse data dictionary file"
                      ", or no <fix> node found at root");
  DOMAttributesPtr attrs = pFixNode->getAttributes();
  std::string type = "FIX";
  if(attrs->get("type", type))
  {
    if(type != "FIX" && type != "FIXT")
      throw ConfigError("type attribute must be FIX or FIXT");
  }
  std::string major;
  if(!attrs->get("major", major))
    throw ConfigError("major attribute not found on <fix>");
  std::string minor;
  if(!attrs->get("minor", minor))
    throw ConfigError("minor attribute not found on <fix>");
  setVersion(type + "." + major + "." + minor);

  // FIELDS
  DOMNodePtr pFieldsNode = pDoc->getNode("/fix/fields");
  if(!pFieldsNode.get())
    throw ConfigError("<fields> section not found in data dictionary");

  DOMNodePtr pFieldNode = pFieldsNode->getFirstChildNode();
  if(!pFieldNode.get()) throw ConfigError("No fields defined");

  while(pFieldNode.get())
  {
    if(pFieldNode->getName() == "field")
    {
      DOMAttributesPtr attrs = pFieldNode->getAttributes();
      std::string name;
      if(!attrs->get("name", name))
        throw ConfigError("<field> does not have a name attribute");
      std::string number;
      if(!attrs->get("number", number))
        throw ConfigError("<field> " + name + " does not have a number attribute");
      int num = atoi(number.c_str());
      std::string type;
      if(!attrs->get("type", type))
        throw ConfigError("<field> " + name + " does not have a type attribute");
      addField(num);
      addFieldType(num, XMLTypeToType(type));
      addFieldName(num, name);

      DOMNodePtr pFieldValueNode = pFieldNode->getFirstChildNode();
      while(pFieldValueNode.get())
      {
        if(pFieldValueNode->getName() == "value")
        {
          DOMAttributesPtr attrs = pFieldValueNode->getAttributes();
          std::string enumeration;
          if(!attrs->get("enum", enumeration))
            throw ConfigError("<value> does not have enum attribute in field " + name);
          addFieldValue(num, enumeration);
          std::string description;
          if(attrs->get("description", description))
            addValueName(num, enumeration, description);
        }
        RESET_AUTO_PTR(pFieldValueNode, pFieldValueNode->getNextSiblingNode());
      }
    }
    RESET_AUTO_PTR(pFieldNode, pFieldNode->getNextSiblingNode());
  }

  // HEADER
  if( type == "FIXT" || (type == "FIX" && major < "5") )
  {
    DOMNodePtr pHeaderNode = pDoc->getNode("/fix/header");
    if(!pHeaderNode.get())
      throw ConfigError("<header> section not found in data dictionary");

    DOMNodePtr pHeaderFieldNode = pHeaderNode->getFirstChildNode();
    if(!pHeaderFieldNode.get()) throw ConfigError("No header fields defined");

    while(pHeaderFieldNode.get())
    {
      if(pHeaderFieldNode->getName() == "field" || pHeaderFieldNode->getName() == "group" )
      {
        DOMAttributesPtr attrs = pHeaderFieldNode->getAttributes();
        std::string name;
        if(!attrs->get("name", name))
          throw ConfigError("<field> does not have a name attribute");
        std::string required = "false";
        attrs->get("required", required);
        addHeaderField(lookupXMLFieldNumber(pDoc.get(), name), required == "true");
      }
      if(pHeaderFieldNode->getName() == "group")
      {
        DOMAttributesPtr attrs = pHeaderFieldNode->getAttributes();
        std::string required;
        attrs->get("required", required);
        bool isRequired = (required == "Y" || required == "y");
        addXMLGroup(pDoc.get(), pHeaderFieldNode.get(), "_header_", *this, isRequired);
      }

      RESET_AUTO_PTR(pHeaderFieldNode, pHeaderFieldNode->getNextSiblingNode());
    }
  }

  // TRAILER
    if( type == "FIXT" || (type == "FIX" && major < "5") )
    {
    DOMNodePtr pTrailerNode = pDoc->getNode("/fix/trailer");
    if(!pTrailerNode.get())
      throw ConfigError("<trailer> section not found in data dictionary");

    DOMNodePtr pTrailerFieldNode = pTrailerNode->getFirstChildNode();
    if(!pTrailerFieldNode.get()) throw ConfigError("No trailer fields defined");

    while(pTrailerFieldNode.get())
    {
      if(pTrailerFieldNode->getName() == "field" || pTrailerFieldNode->getName() == "group" )
      {
        DOMAttributesPtr attrs = pTrailerFieldNode->getAttributes();
        std::string name;
        if(!attrs->get("name", name))
          throw ConfigError("<field> does not have a name attribute");
        std::string required = "false";
        attrs->get("required", required);
        addTrailerField(lookupXMLFieldNumber(pDoc.get(), name), required == "true");
      }
      if(pTrailerFieldNode->getName() == "group")
      {
        DOMAttributesPtr attrs = pTrailerFieldNode->getAttributes();
        std::string required;
        attrs->get("required", required);
        bool isRequired = (required == "Y" || required == "y");
        addXMLGroup(pDoc.get(), pTrailerFieldNode.get(), "_trailer_", *this, isRequired);
      }

      RESET_AUTO_PTR(pTrailerFieldNode, pTrailerFieldNode->getNextSiblingNode());
    }
  }

  // MSGTYPE
  DOMNodePtr pMessagesNode = pDoc->getNode("/fix/messages");
  if(!pMessagesNode.get())
    throw ConfigError("<messages> section not found in data dictionary");

  DOMNodePtr pMessageNode = pMessagesNode->getFirstChildNode();
  if(!pMessageNode.get()) throw ConfigError("No messages defined");

  while(pMessageNode.get())
  {
    if(pMessageNode->getName() == "message")
    {
      DOMAttributesPtr attrs = pMessageNode->getAttributes();
      std::string msgtype;
      if(!attrs->get("msgtype", msgtype))
        throw ConfigError("<field> does not have a name attribute");
      addMsgType(msgtype);

      std::string name;
      if(attrs->get("name", name))
        addValueName( 35, msgtype, name );

      DOMNodePtr pMessageFieldNode = pMessageNode->getFirstChildNode();
      while( pMessageFieldNode.get() )
      {
        if(pMessageFieldNode->getName() == "field"
           || pMessageFieldNode->getName() == "group")
        {
          DOMAttributesPtr attrs = pMessageFieldNode->getAttributes();
          std::string name;
          if(!attrs->get("name", name))
            throw ConfigError("<field> does not have a name attribute");
          int num = lookupXMLFieldNumber(pDoc.get(), name);
          addMsgField(msgtype, num);

          std::string required;
          if(attrs->get("required", required)
             && (required == "Y" || required == "y"))
          {
            addRequiredField(msgtype, num);
          }
        }
        else if(pMessageFieldNode->getName() == "component")
        {
          DOMAttributesPtr attrs = pMessageFieldNode->getAttributes();
          std::string required;
          attrs->get("required", required);
          bool isRequired = (required == "Y" || required == "y");
          addXMLComponentFields(pDoc.get(), pMessageFieldNode.get(),
                                msgtype, *this, isRequired);
        }
        if(pMessageFieldNode->getName() == "group")
        {
          DOMAttributesPtr attrs = pMessageFieldNode->getAttributes();
          std::string required;
          attrs->get("required", required);
          bool isRequired = (required == "Y" || required == "y");
          addXMLGroup(pDoc.get(), pMessageFieldNode.get(), msgtype, *this, isRequired);
        }
        RESET_AUTO_PTR(pMessageFieldNode,
                       pMessageFieldNode->getNextSiblingNode());
      }
    }
    RESET_AUTO_PTR(pMessageNode, pMessageNode->getNextSiblingNode());
  }
}

message_order const&  DataDictionary::getOrderedFields() const
{
  if( m_orderedFieldsArray ) return m_orderedFieldsArray;

  Util::scoped_array<int>::type ordered( new int[m_orderedFields.size() + 1] );
  copy_to_array(m_orderedFields.begin(), m_orderedFields.end(), ordered.get(), m_orderedFields.size() + 1);
  ordered.get()[m_orderedFields.size()] = 0;

  return  (m_orderedFieldsArray = message_order(ordered.get()));
}

int DataDictionary::lookupXMLFieldNumber( DOMDocument* pDoc, DOMNode* pNode ) const
{
  DOMAttributesPtr attrs = pNode->getAttributes();
  std::string name;
  if(!attrs->get("name", name))
    throw ConfigError("No name given to field");
  return lookupXMLFieldNumber( pDoc, name );
}

int DataDictionary::lookupXMLFieldNumber
( DOMDocument* pDoc, const std::string& name ) const
{
  NameToField::const_iterator i = m_names.find(name);
  if( i == m_names.end() )
    throw ConfigError("Field " + name + " not defined in fields section");
  return i->second;
}

int DataDictionary::addXMLComponentFields( DOMDocument* pDoc, DOMNode* pNode,
                                            const std::string& msgtype,
                                            DataDictionary& DD,
                                            bool componentRequired )
{
  int firstField = 0;

  DOMAttributesPtr attrs = pNode->getAttributes();
  std::string name;
  if(!attrs->get("name", name))
    throw ConfigError("No name given to component");

  DOMNodePtr pComponentNode =
    pDoc->getNode("/fix/components/component[@name='" + name + "']");
  if(pComponentNode.get() == 0)
    throw ConfigError("Component not found");

  DOMNodePtr pComponentFieldNode = pComponentNode->getFirstChildNode();
  while(pComponentFieldNode.get())
  {
    if(pComponentFieldNode->getName() == "field"
       || pComponentFieldNode->getName() == "group")
    {
      DOMAttributesPtr attrs = pComponentFieldNode->getAttributes();
      std::string name;
      if(!attrs->get("name", name))
        throw ConfigError("No name given to field");
      int field = lookupXMLFieldNumber(pDoc, name);
      if( firstField == 0 ) firstField = field;

      std::string required;
      if(attrs->get("required", required)
         && (required == "Y" || required =="y")
         && componentRequired)
      {
        addRequiredField(msgtype, field);
      }

      DD.addField(field);
      DD.addMsgField(msgtype, field);
    }
    if(pComponentFieldNode->getName() == "component")
    {
      DOMAttributesPtr attrs = pComponentFieldNode->getAttributes();
      std::string required;
      attrs->get("required", required);
      bool isRequired = (required == "Y" || required == "y");
      addXMLComponentFields(pDoc, pComponentFieldNode.get(),
                            msgtype, DD, isRequired);
    }
    if(pComponentFieldNode->getName() == "group")
    {
      DOMAttributesPtr attrs = pComponentFieldNode->getAttributes();
      std::string required;
      attrs->get("required", required);
      bool isRequired = (required == "Y" || required == "y");
      addXMLGroup(pDoc, pComponentFieldNode.get(), msgtype, DD, isRequired);
    }
    RESET_AUTO_PTR(pComponentFieldNode,
      pComponentFieldNode->getNextSiblingNode());
  }
  return firstField;
}

void DataDictionary::addXMLGroup( DOMDocument* pDoc, DOMNode* pNode,
                                  const std::string& msgtype,
                                  DataDictionary& DD, bool groupRequired  )
{
  DOMAttributesPtr attrs = pNode->getAttributes();
  std::string name;
  if(!attrs->get("name", name))
    throw ConfigError("No name given to group");
  int group = lookupXMLFieldNumber( pDoc, name );
  int delim = 0;
  int field = 0;
  DataDictionary groupDD;
  DOMNodePtr node = pNode->getFirstChildNode();
  while(node.get())
  {
    if( node->getName() == "field" )
    {
      field = lookupXMLFieldNumber( pDoc, node.get() );
      groupDD.addField( field );

      DOMAttributesPtr attrs = node->getAttributes();
      std::string required;
      if( attrs->get("required", required)
         && ( required == "Y" || required =="y" )
         && groupRequired )
      {
        groupDD.addRequiredField(msgtype, field);
      }
    }
    else if( node->getName() == "component" )
    {
      field = addXMLComponentFields( pDoc, node.get(), msgtype, groupDD, false );
    }
    else if( node->getName() == "group" )
    {
      field = lookupXMLFieldNumber( pDoc, node.get() );
      groupDD.addField( field );
      DOMAttributesPtr attrs = node->getAttributes();
      std::string required;
      if( attrs->get("required", required )
         && ( required == "Y" || required =="y" )
         && groupRequired)
      {
        groupDD.addRequiredField(msgtype, field);
      }
      bool isRequired = false;
      if( attrs->get("required", required) )
      isRequired = (required == "Y" || required == "y");
      addXMLGroup( pDoc, node.get(), msgtype, groupDD, isRequired );
    }
    if( delim == 0 ) delim = field;
    RESET_AUTO_PTR(node, node->getNextSiblingNode());
  }

  if( delim ) DD.addGroup( msgtype, group, delim, groupDD );
}

TYPE::Type DataDictionary::XMLTypeToType( const std::string& type ) const
{
  if ( m_beginString < "FIX.4.2" && type == "CHAR" )
    return TYPE::String;

  if ( type == "STRING" ) return TYPE::String;
  if ( type == "CHAR" ) return TYPE::Char;
  if ( type == "PRICE" ) return TYPE::Price;
  if ( type == "INT" ) return TYPE::Int;
  if ( type == "AMT" ) return TYPE::Amt;
  if ( type == "QTY" ) return TYPE::Qty;
  if ( type == "CURRENCY" ) return TYPE::Currency;
  if ( type == "MULTIPLEVALUESTRING" ) return TYPE::MultipleValueString;
  if ( type == "MULTIPLESTRINGVALUE" ) return TYPE::MultipleStringValue;
  if ( type == "MULTIPLECHARVALUE" ) return TYPE::MultipleCharValue;
  if ( type == "EXCHANGE" ) return TYPE::Exchange;
  if ( type == "UTCTIMESTAMP" ) return TYPE::UtcTimeStamp;
  if ( type == "BOOLEAN" ) return TYPE::Boolean;
  if ( type == "LOCALMKTDATE" ) return TYPE::LocalMktDate;
  if ( type == "DATA" ) return TYPE::Data;
  if ( type == "FLOAT" ) return TYPE::Float;
  if ( type == "PRICEOFFSET" ) return TYPE::PriceOffset;
  if ( type == "MONTHYEAR" ) return TYPE::MonthYear;
  if ( type == "DAYOFMONTH" ) return TYPE::DayOfMonth;
  if ( type == "UTCDATE" ) return TYPE::UtcDate;
  if ( type == "UTCDATEONLY" ) return TYPE::UtcDateOnly;
  if ( type == "UTCTIMEONLY" ) return TYPE::UtcTimeOnly;
  if ( type == "NUMINGROUP" ) return TYPE::NumInGroup;
  if ( type == "PERCENTAGE" ) return TYPE::Percentage;
  if ( type == "SEQNUM" ) return TYPE::SeqNum;
  if ( type == "LENGTH" ) return TYPE::Length;
  if ( type == "COUNTRY" ) return TYPE::Country;
  if ( type == "TIME" ) return TYPE::UtcTimeStamp;
  return TYPE::Unknown;
}

void DataDictionary::addField( int field )
{
  m_fields.insert( field );
  m_orderedFields.push_back( field );
}

void DataDictionary::addFieldName( int field, const std::string& name )
{
  if( m_names.insert( std::make_pair(name, field) ).second == false )
    throw ConfigError( "Field named " + name + " defined multiple times" );
  m_fieldNames[field] = name;
}

void DataDictionary::addValueName( int field, const std::string& value, const std::string& name )
{
  m_valueNames[std::make_pair(field, value)] = name;
}

void DataDictionary::addMsgType( const std::string& msgType )
{
  m_messages.insert( String::value_type( msgType.c_str(), msgType.length() ) );
}

void DataDictionary::addMsgField( const std::string& msgType, int field )
{
  MsgTypeToField::iterator i = m_messageFields.find( msgType );
  if ( i == m_messageFields.end() )
    m_messageFields.insert(
      MsgTypeToField::value_type(
        String::value_type( msgType.c_str(), msgType.length() ),
          MsgFields( MsgFields::key_compare(), get_allocator<MsgFields>()) ) );
  m_messageFields[ msgType ].insert( field );
}

void DataDictionary::addHeaderField( int field, bool required )
{
  m_headerFields[ field ] = required;
  if ( field < HeaderTypeBits )
      m_fieldTypeHeader.set(field);
}

void DataDictionary::addTrailerField( int field, bool required )
{
  m_trailerFields[ field ] = required;
  if ( field < TrailerTypeBits )
      m_fieldTypeTrailer.set(field);
}

void DataDictionary::addFieldType( int field, FIX::TYPE::Type type )
{
  m_fieldTypes[ field ] = type;
  if (field < DataTypeBits && type == TYPE::Data)
      m_fieldTypeData.set(field);
}

void DataDictionary::addRequiredField( const std::string& msgType, int field )
{
  MsgTypeToField::iterator i = m_requiredFields.find( msgType );
  if ( i == m_requiredFields.end() )
    m_requiredFields.insert(
      MsgTypeToField::value_type(
        String::value_type( msgType.c_str(), msgType.length() ),
          MsgFields( MsgFields::key_compare(), get_allocator<MsgFields>()) ) );
  m_requiredFields[ msgType ].insert( field );
}

void DataDictionary::addFieldValue( int field, const String::value_type& value )
{
  FieldToValue::iterator i = m_fieldValues.find( field );
  if (i == m_fieldValues.end())
    m_fieldValues.insert(
      FieldToValue::value_type(
        field, Values( get_allocator<Values>()) ) );
  m_fieldValues[ field ].insert( value );
}

void DataDictionary::addGroup( const FieldPresenceMap::key_type& msg, int field, int delim,
               const DataDictionary& dataDictionary )
{
  DataDictionary * pDD = new DataDictionary( dataDictionary );
  pDD->setVersion( getVersion() );

  if ( field < GroupTypeBits ) m_fieldTypeGroup.set(field);
  FieldToGroup::iterator it = m_groups.find( field );
  if ( it == m_groups.end() )
    m_groups.insert(
      FieldToGroup::value_type( field,
        FieldPresenceMap( get_allocator<FieldPresenceMap>()) ) ); 
  FieldPresenceMap& presenceMap = m_groups[ field ];
  presenceMap[ String::CopyFunc()(msg) ] = std::make_pair( delim, pDD );
}
}
