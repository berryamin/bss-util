// Copyright �2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#ifndef __C_INISTORAGE_H__BSS__
#define __C_INISTORAGE_H__BSS__

#include "cINIsection.h"

namespace bss_util {
  // Stores an INI file as a linked list of sections, also stored via hash for O(1) time operations.
  class BSS_DLLEXPORT cINIstorage
  {
  public:
    typedef _INInode<cINIsection> _NODE;

    // Constructors
    cINIstorage(const cINIstorage& copy);
    cINIstorage(cINIstorage&& mov);
    cINIstorage(const char* file=0, std::ostream* logger=0);
    cINIstorage(FILE* file, std::ostream* logger=0);
    // Destructor
    ~cINIstorage();
    // Gets a section based on name and instance
    cINIsection* GetSection(const char* section, size_t instance=0) const;
    // Gets number of sections with the given name
    size_t GetNumSections(const char* section) const;
    // Gets a section node based on name and instance
    _NODE* GetSectionNode(const char* section, size_t instance=0) const;
    // Gets a convertable INI entry
    cINIentry* GetEntryPtr(const char *section, const char* key, size_t keyinstance=0, size_t secinstance=0) const;
    // Gets an INI entry node for iteration
    cINIsection::_NODE* GetEntryNode(const char *section, const char* key, size_t keyinstance=0, size_t secinstance=0) const;
    // Gets the root node of the section linked list
    inline const _NODE* Front() const { return _root; }
    // Gets the last node of the section linked list
    inline const _NODE* Back() const { return _last; }
    // iterators for standard containers
    inline cINIsection::INIiterator<cINIsection> begin() { return cINIsection::INIiterator<cINIsection>(_root); }
    inline cINIsection::INIiterator<cINIsection> end() { return cINIsection::INIiterator<cINIsection>(0); }

    cINIsection& AddSection(const char* name);
    bool RemoveSection(const char* name, size_t instance=0);
    char EditEntry(const char* section, const char* key, const char* nvalue=0, size_t keyinstance=0, size_t secinstance=0); //if nvalue is 0 the entry is deleted. if either instance is -1 it triggers an insert
    inline char EditAddEntry(const char* section, const char* key, const char* nvalue=0, uint32_t keyinstance=0,uint32_t secinstance=0) { return EditEntry(section,key,nvalue,GetEntryPtr(section,key,keyinstance,secinstance)==0?-1:keyinstance,GetSection(section,secinstance)==0?-1:secinstance); }
    void EndINIEdit(const char* overridepath=0); //Saves changes to file (INI files are automatically opened when an edit operation is done)
    void DiscardINIEdit();

    BSS_FORCEINLINE cINIsection& operator [](const char *section) const { cINIsection* ret=GetSection(section,0); return !ret?_sectionsentinel:*ret; }
    cINIstorage& operator=(const cINIstorage& right);
    cINIstorage& operator=(cINIstorage&& mov);
    inline const char* GetPath() const { return _path; } //gets path to folder this INI was in
    inline cINIentry& GetEntry(const char *section, const char* key, uint32_t keyinstance=0, uint32_t secinstance=0) const { cINIentry* ret=GetEntryPtr(section,key,keyinstance,secinstance); return !ret?cINIsection::_entrysentinel:*ret; }
    
  protected:
    friend class cINIsection;

    void _loadINI(FILE* file);
    void _openINI();
    void _setfilepath(const char* path);
    cINIsection* _addsection(const char* name);
    void _copy(const cINIstorage& copy);
    void _destroy();

    static cINIsection _sectionsentinel;
    static cLocklessBlockAlloc<_NODE> _alloc;

    cHash<const char*, _NODE*, true> _sections;
    cStr _path; //holds path to INI
    cStr _filename; //holds INI filename;
    cStr* _ini; //holds entire INI file. Made a pointer so we can distinguish between an empty INI file and an unopened file.
    std::ostream* _logger; //Holds a pointer to a logger object
    _NODE* _root;
    _NODE* _last;
  };
}

#endif