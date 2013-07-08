#ifndef  SDAIHEADER_SECTION_SCHEMA_H
#define  SDAIHEADER_SECTION_SCHEMA_H
// This file was generated by fedex_plus.  You probably don't want to edit
// it since your modifications will be lost if fedex_plus is used to
// regenerate it.

#include <sc_export.h>
#include <sdai.h>
#include <Registry.h>
#include <STEPaggregate.h>
#include <SdaiHeaderSchemaClasses.h>
#include <SdaiSchemaInit.h>

/////////         ENTITY section_language

extern SC_EDITOR_EXPORT AttrDescriptor * a_0section;
extern SC_EDITOR_EXPORT AttrDescriptor * a_1default_language;

class SC_EDITOR_EXPORT SdaiSection_language  :    public SDAI_Application_instance {
    protected:
        SDAI_String _section ;    //  OPTIONAL
        SDAI_String _default_language ;
    public:

        SdaiSection_language( );
        SdaiSection_language( SDAI_Application_instance * se, int * addAttrs = 0 );
        SdaiSection_language( SdaiSection_language & e );
        ~SdaiSection_language();

        int opcode()  {
            return 0 ;
        }
        const SdaiSection_name section_() const;
        void section_( const SdaiSection_name x );

        const SdaiLanguage_name default_language_() const;
        void default_language_( const SdaiLanguage_name x );

};

inline SdaiSection_language *
create_SdaiSection_language() {
    return  new SdaiSection_language ;
}

/////////         END_ENTITY section_language


/////////         ENTITY file_population

extern SC_EDITOR_EXPORT AttrDescriptor * a_2governing_schema;
extern SC_EDITOR_EXPORT AttrDescriptor * a_3determination_method;
extern SC_EDITOR_EXPORT AttrDescriptor * a_4governed_sections;

class SC_EDITOR_EXPORT SdaiFile_population  :    public SDAI_Application_instance {
    protected:
        SDAI_String _governing_schema ;
        SDAI_String _determination_method ;
        StringAggregate _governed_sections ;    //  OPTIONAL          //  of  section_name

    public:

        SdaiFile_population( );
        SdaiFile_population( SDAI_Application_instance * se, int * addAttrs = 0 );
        SdaiFile_population( SdaiFile_population & e );
        ~SdaiFile_population();

        int opcode()  {
            return 1 ;
        }
        const SdaiSchema_name governing_schema_() const;
        void governing_schema_( const SdaiSchema_name x );

        const SdaiExchange_structure_identifier determination_method_() const;
        void determination_method_( const SdaiExchange_structure_identifier x );

        const StringAggregate_ptr governed_sections_() const;
        void governed_sections_( const StringAggregate_ptr x );

};

inline SdaiFile_population *
create_SdaiFile_population() {
    return  new SdaiFile_population ;
}

/////////         END_ENTITY file_population


/////////         ENTITY file_name

extern AttrDescriptor * a_5name;
extern AttrDescriptor * a_6time_stamp;
extern AttrDescriptor * a_7author;
extern AttrDescriptor * a_8organization;
extern AttrDescriptor * a_9preprocessor_version;
extern AttrDescriptor * a_10originating_system;
extern AttrDescriptor * a_11authorization;

class SC_EDITOR_EXPORT SdaiFile_name  :    public SDAI_Application_instance {
    protected:
        SDAI_String _name ;
        SDAI_String _time_stamp ;
        StringAggregate _author ;
        StringAggregate _organization ;
        SDAI_String _preprocessor_version ;
        SDAI_String _originating_system ;
        SDAI_String _authorization ;
    public:

        SdaiFile_name( );
        SdaiFile_name( SDAI_Application_instance * se, int * addAttrs = 0 );
        SdaiFile_name( SdaiFile_name & e );
        ~SdaiFile_name();

        int opcode()  {
            return 2 ;
        }
        const SDAI_String name_() const;
        void name_( const SDAI_String x );

        const SdaiTime_stamp_text time_stamp_() const;
        void time_stamp_( const SdaiTime_stamp_text x );

        const StringAggregate_ptr author_() const;
        void author_( const StringAggregate_ptr x );

        const StringAggregate_ptr organization_() const;
        void organization_( const StringAggregate_ptr x );

        const SDAI_String preprocessor_version_() const;
        void preprocessor_version_( const SDAI_String x );

        const SDAI_String originating_system_() const;
        void originating_system_( const SDAI_String x );

        const SDAI_String authorization_() const;
        void authorization_( const SDAI_String x );

};

inline SdaiFile_name *
create_SdaiFile_name() {
    return  new SdaiFile_name ;
}

/////////         END_ENTITY file_name


/////////         ENTITY section_context

extern SC_EDITOR_EXPORT AttrDescriptor * a_12section;
extern SC_EDITOR_EXPORT AttrDescriptor * a_13context_identifiers;

class SC_EDITOR_EXPORT SdaiSection_context  :    public SDAI_Application_instance {
    protected:
        SDAI_String _section ;    //  OPTIONAL
        StringAggregate _context_identifiers ;          //  of  context_name

    public:

        SdaiSection_context( );
        SdaiSection_context( SDAI_Application_instance * se, int * addAttrs = 0 );
        SdaiSection_context( SdaiSection_context & e );
        ~SdaiSection_context();

        int opcode()  {
            return 3 ;
        }
        const SdaiSection_name section_() const;
        void section_( const SdaiSection_name x );

        const StringAggregate_ptr context_identifiers_() const;
        void context_identifiers_( const StringAggregate_ptr x );

};

inline SdaiSection_context *
create_SdaiSection_context() {
    return  new SdaiSection_context ;
}

/////////         END_ENTITY section_context


/////////         ENTITY file_description

extern SC_EDITOR_EXPORT AttrDescriptor * a_14description;
extern SC_EDITOR_EXPORT AttrDescriptor * a_15implementation_level;

class SC_EDITOR_EXPORT SdaiFile_description  :    public SDAI_Application_instance {
    protected:
        StringAggregate _description ;
        SDAI_String _implementation_level ;
    public:

        SdaiFile_description( );
        SdaiFile_description( SDAI_Application_instance * se, int * addAttrs = 0 );
        SdaiFile_description( SdaiFile_description & e );
        ~SdaiFile_description();

        int opcode()  {
            return 4 ;
        }
        const StringAggregate_ptr description_() const;
        void description_( const StringAggregate_ptr x );

        const SDAI_String implementation_level_() const;
        void implementation_level_( const SDAI_String x );

};

inline SdaiFile_description *
create_SdaiFile_description() {
    return  new SdaiFile_description ;
}

/////////         END_ENTITY file_description


/////////         ENTITY file_schema

extern SC_EDITOR_EXPORT AttrDescriptor * a_16schema_identifiers;

class SC_EDITOR_EXPORT SdaiFile_schema  :    public SDAI_Application_instance {
    protected:
        StringAggregate _schema_identifiers ;          //  of  schema_name

    public:

        SdaiFile_schema( );
        SdaiFile_schema( SDAI_Application_instance * se, int * addAttrs = 0 );
        SdaiFile_schema( SdaiFile_schema & e );
        ~SdaiFile_schema();

        int opcode()  {
            return 5 ;
        }
        const StringAggregate_ptr schema_identifiers_() const;
        void schema_identifiers_( const StringAggregate_ptr x );

};

inline SdaiFile_schema *
create_SdaiFile_schema() {
    return  new SdaiFile_schema ;
}

/////////         END_ENTITY file_schema


//        ***** generate Model related pieces

class SC_EDITOR_EXPORT SdaiModel_contents_header_section_schema : public SDAI_Model_contents {

    public:
        SdaiModel_contents_header_section_schema();

        SdaiSection_language__set_var SdaiSection_language_get_extents();

        SdaiFile_population__set_var SdaiFile_population_get_extents();

        SdaiFile_name__set_var SdaiFile_name_get_extents();

        SdaiSection_context__set_var SdaiSection_context_get_extents();

        SdaiFile_description__set_var SdaiFile_description_get_extents();

        SdaiFile_schema__set_var SdaiFile_schema_get_extents();

};


typedef SdaiModel_contents_header_section_schema * SdaiModel_contents_header_section_schema_ptr;
typedef SdaiModel_contents_header_section_schema_ptr SdaiModel_contents_header_section_schema_var;
SC_EDITOR_EXPORT SDAI_Model_contents_ptr create_SdaiModel_contents_header_section_schema();
#endif
