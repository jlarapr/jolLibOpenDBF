/*
Original Source Code
db2txt
Copyright (c) 2011 Bruno Sanches  http://code.google.com/p/dbf2txt
This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "dbffile.h"

#include <iostream>
#include <sstream>
#include <cstring>

using namespace std;
using namespace std::string_literals;


DbfFile_c::DbfFile_c(const char *szFileName): clFile(szFileName, std::ios_base::binary | std::ios_base::in)
{
    if(!clFile.good())
        throw std::invalid_argument("Cannot open file");

    clFile.read(reinterpret_cast<char *>(&stHeader), sizeof(stHeader));
    //size_t sz = sizeof(DbfRecord_s);

    const auto numRecords = stHeader.uNumRecords;

    cout << "numRecords:" << numRecords << endl;

    szRowSize = 0;
    szLargestFieldSize = 0;

    //    for(;;)
    for(unsigned i = 0;i < numRecords; ++i)
    {
        char end;
        clFile.read(&end, 1);

        if(end == 0x0D)
            break;

        //corrupted file? Abort to avoid infinite loop
        if (i == numRecords)
            break;

        vecRecords.push_back(DbfRecord_s());
        DbfRecord_s &record = vecRecords.back();

        memcpy(&record, &end, 1);
        clFile.read(reinterpret_cast<char *>(&record)+1, sizeof(DbfRecord_s)-1);

        szRowSize += record.uLength;
        szLargestFieldSize = std::max(szLargestFieldSize, static_cast<size_t>(record.uLength));
    }
    tblName("tmp");
}

void DbfFile_c::DumpAll(const char *szDestFileName)
{
    std::ofstream out(szDestFileName);
    std::vector<char> vecBuffer;
    vecBuffer.resize(szLargestFieldSize);

    size_t uTotalBytes = 0;
    size_t uNumRecords = 0;
    bool isStart = true;

    while(!clFile.eof())
    {
        char deleted;
        clFile.read(&deleted, 1);

        if(deleted == 0x2A)
        {
            clFile.seekg(static_cast<int>(szRowSize), std::ios_base::cur);
            continue;
        }

        if(clFile.fail())
            break;

        if (deleted == 0x1A) //end-of-file
            break;

        if (isStart) {
            for(size_t i = 0;i < vecRecords.size(); ++i)
            {
                DbfRecord_s &record = vecRecords[i];
                string s = record.archName;
                int len = static_cast<int>(s.length());
                out.write(record.archName,len);
                out.write("|", 1);
            }
            out.write("\r\n",1);
            isStart = false;
        }

        for(size_t i = 0;i < vecRecords.size(); ++i)
        {
            DbfRecord_s &record = vecRecords[i];

            clFile.read(&vecBuffer[0], record.uLength);
            out.write(&vecBuffer[0], record.uLength);
            out.write("|", 1);
            uTotalBytes += record.uLength;
        }
        ++uNumRecords;
        ++uTotalBytes;

        out << std::endl;
    }
    std::cout << "Created " << uNumRecords << ", records " << uTotalBytes << " bytes." << std::endl;
}

void DbfFile_c::DumpAllAndCreateSqlFile(const char *szDestFileName,const char *szDestFileNameInsert,std::string primaryKeyField)
{//this method creates two .sql files, first file create table,second file insert data
    std::ofstream out(szDestFileName);
    std::ofstream outInsert(szDestFileNameInsert);

    std::vector<char> vecBuffer;
    vecBuffer.resize(szLargestFieldSize);

    size_t uTotalBytes = 0;
    size_t uNumRecords = 0;
    bool isStart = true;

    if (tblName() == "")
        tblName("tmp");

    string sqlInsert = "";
    string sqlInsertTmp ="INSERT INTO " + tblName()  +"(";
    bool starLoop = true;

    while(!clFile.eof())
    {
        char deleted;

        clFile.read(&deleted, 1);

        sqlInsert = sqlInsertTmp;

        if(deleted == 0x2A)
        {
            clFile.seekg(static_cast<int>(szRowSize), std::ios_base::cur);
            continue;
        }

        if(clFile.fail())
            break;

        if (deleted == 0x1A)//end-of-file
            break;

        //if (deleted == 0xf5) {
        //    cout << "FoxPro w. memo file" << endl;
        //}

        if (isStart) {
            string sqlCreate = "CREATE TABLE " + tblName() + "(";

            string sqlFilsType = " TEXT";

            out.write(sqlCreate.c_str(),static_cast<int>(sqlCreate.length()));

            starLoop = true;

            for(size_t i = 0;i < vecRecords.size(); ++i)
            {
                DbfRecord_s &record = vecRecords[i];
                string s = record.archName;
                int len = static_cast<int>(s.length());

                if (!starLoop)
                {
                    out.write(",",1);
                    sqlInsert += ",";
                }

                starLoop = false;

                if (strcmp(record.archName,primaryKeyField.c_str()) == 0) {
                    sqlFilsType = " TEXT NOT NULL";
                }else {
                    sqlFilsType = " TEXT";
                }
                out.write(record.archName,len);
                out.write(sqlFilsType.c_str(),static_cast<int>(sqlFilsType.length()));
                sqlInsert += trim(s);
                sqlInsertTmp = sqlInsert;
                //outInsert.write(record.archName,len);
            }

            out.write(",",1);
            primaryKeyField.insert(0,"PRIMARY KEY(");
            primaryKeyField += ")";
            out.write(primaryKeyField.c_str() ,static_cast<int>(primaryKeyField.length()));
            out.write(");",2);
            out.flush();
            out.close();
            isStart = false;
        }
        sqlInsert +=") VALUES('";
        outInsert.write(sqlInsert.c_str(),static_cast<int>(sqlInsert.length()));
        sqlInsert = "";
        starLoop = true;

        for(size_t i = 0;i < vecRecords.size(); ++i)
        {
            DbfRecord_s &record = vecRecords[i];

            clFile.read(&vecBuffer[0], record.uLength);

            if (!starLoop)
                outInsert.write(",'",2);

            outInsert.write(&vecBuffer[0], record.uLength);
            outInsert.write("'", 1);

            uTotalBytes += record.uLength;
            starLoop = false;
        }
        outInsert.write(");",2);
        ++uNumRecords;
        ++uTotalBytes;

        outInsert << endl;
        outInsert.flush();
    }
    std::cout << "Created " << uNumRecords << ", records " << uTotalBytes << " bytes." << std::endl;
    outInsert.close();
}

struct FieldInfo_s
{
    const DbfRecord_s &rstRecord;
    size_t szSkipSize;

    FieldInfo_s(const DbfRecord_s &rec):
        rstRecord(rec),
        szSkipSize(0)
    {
    }

    FieldInfo_s &operator=(const FieldInfo_s &rhs)
    {
        return *this;
    }
};

void DbfFile_c::DumpFields(const char *szDestFileName, const char **fields, size_t numFields)
{
    std::vector<FieldInfo_s> vecFields;
    vecFields.reserve(numFields);

    cout << vecRecords.size() << endl;
    cout << numFields << endl;

    //Build a sorted list (in file fields ordering) with all fields
    size_t current = 0;
    for(size_t i = 0;(i < vecRecords.size()) && (current < numFields); ++i)
    {
        cout << vecRecords[i].archName << endl;
        cout << fields[current] << endl;

        if(strncmp(vecRecords[i].archName, fields[current], 11) == 0)
        {
            vecFields.push_back(FieldInfo_s(vecRecords[i]));
            ++current;
        }
    }

    if(current < numFields)
    {
        std::stringstream stream;
        stream << "Field not found: " << fields[current];
        throw std::invalid_argument(stream.str().c_str());
    }

    //Now build the skip table
    current = 0;
    size_t szEndOfRowSeek = 0;
    for(size_t i = 0;i < numFields; ++i)
    {
        for(;current < vecRecords.size(); ++current)
        {
            if(&vecFields[i].rstRecord == &vecRecords[current])
            {
                szEndOfRowSeek += vecFields[i].szSkipSize;
                szEndOfRowSeek += vecFields[i].rstRecord.uLength;
                ++current;
                break;
            }

            vecFields[i].szSkipSize += vecRecords[current].uLength;
        }
    }
    szEndOfRowSeek = szEndOfRowSeek == szRowSize ? 0 : szRowSize - szEndOfRowSeek;

    //Finally, do the output work
    std::ofstream out(szDestFileName);

    std::vector<char> vecBuffer;
    vecBuffer.resize(szLargestFieldSize);

    size_t uTotalBytes = 0;
    size_t uNumRecords = 0;
    while(uNumRecords < stHeader.uNumRecords)
    {
        char deleted;
        clFile.read(&deleted, 1);
        if(deleted == 0x2A)
        {
            clFile.seekg(static_cast<int>(szRowSize), std::ios_base::cur);
            continue;
        }

        for(size_t i = 0;i < numFields; ++i)
        {
            if(vecFields[i].szSkipSize > 0)
            {
                clFile.seekg(static_cast<int>(vecFields[i].szSkipSize), std::ios_base::cur);
            }

            const DbfRecord_s &record = vecFields[i].rstRecord;

            clFile.read(&vecBuffer[0], record.uLength);
            out.write(&vecBuffer[0], record.uLength);

            uTotalBytes += record.uLength;
        }

        if(szEndOfRowSeek > 0)
            clFile.seekg(static_cast<int>(szEndOfRowSeek), std::ios_base::cur);

        out << std::endl;
        ++uNumRecords;
        ++uTotalBytes;
    }

    std::cout << "Created " << uNumRecords << ", records " << uTotalBytes << " bytes." << std::endl;
}

string DbfFile_c::tblName() const
{
    return _tblName;
}

void DbfFile_c::tblName(string v)
{
    _tblName = v;
}

string &DbfFile_c::ltrim(string &str, const string &chars)
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}

string &DbfFile_c::rtrim(string &str, const string &chars)
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

string &DbfFile_c::trim(string &str, const string &chars)
{
    return ltrim(rtrim(str, chars), chars);
}
