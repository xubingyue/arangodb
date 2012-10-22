////////////////////////////////////////////////////////////////////////////////
/// @brief csv functions
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2004-2012 triagens GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Copyright 2011-2012, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "csv.h"

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup CSV
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief inits a CSV parser
////////////////////////////////////////////////////////////////////////////////

void TRI_InitCsvParser (TRI_csv_parser_t* parser,
                        TRI_memory_zone_t* zone,
                        void (*begin) (TRI_csv_parser_t*, size_t),
                        void (*add) (TRI_csv_parser_t*, char const*, size_t, size_t, bool),
                        void (*end) (TRI_csv_parser_t*, char const*, size_t, size_t, bool)) {
  size_t length;

  parser->_state = TRI_CSV_PARSER_BOL;

  TRI_SetQuoteCsvParser(parser, '"', true);
  TRI_SetEolCsvParser(parser, "\n", 1);
  TRI_SetSeparatorCsvParser(parser, ";", 1);

  length = 1024;

  parser->_row = 0;
  parser->_column = 0;

  parser->_memoryZone = zone;

  parser->_begin = TRI_Allocate(zone, length, false);

  if (parser->_begin == NULL) {
    length = 0;
  }

  parser->_start = parser->_begin;
  parser->_written = parser->_begin;
  parser->_current = parser->_begin;
  parser->_stop = parser->_begin;
  parser->_end = parser->_begin + length;

  parser->_dataBegin = NULL;
  parser->_dataAdd = NULL;
  parser->_dataEnd = NULL;

  parser->begin = begin;
  parser->add = add;
  parser->end = end;

  parser->_nResize = 0;
  parser->_nMemmove = 0;
  parser->_nMemcpy = 0;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief destroys a CSV parser
////////////////////////////////////////////////////////////////////////////////

void TRI_DestroyCsvParser (TRI_csv_parser_t* parser) {
  if (parser->_begin != NULL) {
    TRI_Free(parser->_memoryZone, parser->_begin);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup CSV
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief set the separator
///
/// note that the separator string must be valid until the parser is destroyed
////////////////////////////////////////////////////////////////////////////////

void TRI_SetSeparatorCsvParser (TRI_csv_parser_t* parser, 
                                char* separator, 
                                size_t separatorLength) {
  parser->_separator = separator;
  parser->_separatorLength = separatorLength;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief set the eol character(s)
///
/// note that the eol string must be valid until the parser is destroyed
////////////////////////////////////////////////////////////////////////////////

void TRI_SetEolCsvParser (TRI_csv_parser_t* parser, 
                          char* eol,
                          size_t eolLength) {
  parser->_eol = eol;
  parser->_eolLength = eolLength;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief set the quote character
////////////////////////////////////////////////////////////////////////////////

void TRI_SetQuoteCsvParser (TRI_csv_parser_t* parser, 
                            char quote,
                            bool useQuote) {
  parser->_quote = quote;
  parser->_useQuote = useQuote;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief parses a CSV line
////////////////////////////////////////////////////////////////////////////////

int TRI_ParseCsvString (TRI_csv_parser_t* parser, char const* line) {
  return TRI_ParseCsvString2(parser, line, strlen(line));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief parses a CSV line
////////////////////////////////////////////////////////////////////////////////

int TRI_ParseCsvString2 (TRI_csv_parser_t* parser, char const* line, size_t length) {
  char* ptr;
  char* qtr;

  // append line to buffer
  if (line != NULL) {
    size_t l1;
    size_t l2;
    size_t l3;
    size_t l4;
    size_t l5;

    assert(parser->_begin <= parser->_start);
    assert(parser->_start <= parser->_written);
    assert(parser->_written <= parser->_current);
    assert(parser->_current <= parser->_stop);
    assert(parser->_stop <= parser->_end);

    // there is enough room between STOP and END
    if (parser->_stop + length <= parser->_end) {
      memcpy(parser->_stop, line, length);

      parser->_stop += length;
      parser->_nMemcpy++;
    }
    else {
      l1 = parser->_start - parser->_begin;
      l2 = parser->_end - parser->_stop;

      // not enough room, but enough room between BEGIN and START plus STOP and END
      if (length <= l1 + l2) {
        l3 = parser->_stop - parser->_start;

        if (0 < l3) {
          memmove(parser->_begin, parser->_start, l3);
        }

        memcpy(parser->_begin + l3, line, length);

        parser->_start = parser->_begin;
        parser->_written = parser->_written - l1;
        parser->_current = parser->_current - l1;
        parser->_stop = parser->_begin + l3 + length;
        parser->_nMemmove++;
      }

      // really not enough room
      else {
        l2 = parser->_stop - parser->_start;
        l3 = parser->_end - parser->_begin + length;
        l4 = parser->_written - parser->_start;
        l5 = parser->_current - parser->_start;

        ptr = TRI_Allocate(parser->_memoryZone, l3, false);

        if (ptr == NULL) {
          return TRI_ERROR_OUT_OF_MEMORY;
        }

        memcpy(ptr, parser->_start, l2);
        memcpy(ptr + l2, line, length);
        TRI_Free(parser->_memoryZone, parser->_begin);

        parser->_begin = ptr;
        parser->_start = ptr;
        parser->_written = ptr + l4;
        parser->_current = ptr + l5;
        parser->_stop = ptr + l2 + length;
        parser->_end = ptr + l3;
        parser->_nResize++;
      }
    }

    // start parsing or continue
    ptr = parser->_current;
    qtr = parser->_written;

    while (true) {
      switch (parser->_state) {
        case TRI_CSV_PARSER_BOL:
          if (ptr == parser->_stop) {
            parser->_written = ptr;
            parser->_current = ptr;
            return false;
          }

          parser->begin(parser, parser->_row);

          parser->_column = 0;
          parser->_state = TRI_CSV_PARSER_BOF;

          break;

        case TRI_CSV_PARSER_BOF:
          if (ptr == parser->_stop) {
            parser->_written = ptr;
            parser->_current = ptr;
            return TRI_ERROR_CORRUPTED_CSV;
          }

          if (parser->_useQuote && *ptr == parser->_quote) {
            if (ptr + 1 == parser->_stop) {
              parser->_written = qtr;
              parser->_current = ptr;
              return TRI_ERROR_CORRUPTED_CSV;
            }

            parser->_state = TRI_CSV_PARSER_WITHIN_QUOTED_FIELD;
            parser->_start = ++ptr;

            qtr = parser->_written = ptr;
          }
          else {
            parser->_state = TRI_CSV_PARSER_WITHIN_FIELD;
            parser->_start = ptr;

            qtr = parser->_written = ptr;
          }

          break;

        case TRI_CSV_PARSER_CORRUPTED:
          while (ptr < parser->_stop) {
            size_t checkLength;
           
            // check for separator
            checkLength = parser->_separatorLength;
            if (parser->_stop - ptr >= checkLength) { 
              // must have at least as much chars as length of separator
              if (0 == memcmp(ptr, parser->_separator, checkLength)) {
                // found separator
                break;
              }
            }

            // check for eol
            checkLength = parser->_eolLength;
            if (parser->_stop - ptr >= checkLength) { 
              // must have at least as much chars as length of eol
              if (0 == memcmp(ptr, parser->_eol, checkLength)) {
                // found eol
                break;
              }
            }

            ptr++;
          }

          // found separator or eol
          if (ptr < parser->_stop) {
            // found separator?
            if (strstr(ptr, parser->_separator) == ptr)  {
              ptr += parser->_separatorLength;

              parser->_state = TRI_CSV_PARSER_BOF;
            }

            // found eol
            else {
              ptr += parser->_eolLength;

              parser->_row++;
              parser->_state = TRI_CSV_PARSER_BOL;
            }
          }

          // need more input
          else {
            parser->_written = qtr;
            parser->_current = ptr;
            return TRI_ERROR_NO_ERROR;
          }

          break;

        case TRI_CSV_PARSER_WITHIN_FIELD:
          while (ptr < parser->_stop) {
            size_t checkLength;
           
            // check for separator
            checkLength = parser->_separatorLength;
            if (parser->_stop - ptr >= checkLength) { 
              // must have at least as much chars as length of separator
              if (0 == memcmp(ptr, parser->_separator, checkLength)) {
                // found separator
                break;
              }
            }

            // check for eol
            checkLength = parser->_eolLength;
            if (parser->_stop - ptr >= checkLength) { 
              // must have at least as much chars as length of eol
              if (0 == memcmp(ptr, parser->_eol, checkLength)) {
                // found eol
                break;
              }
            }

            *qtr++ = *ptr++;
          }

          // found separator or eol
          if (ptr < parser->_stop) {

            // found separator
            if (ptr + parser->_separatorLength < parser->_stop && memcmp(ptr, parser->_separator, parser->_separatorLength) == 0) {
              *qtr = '\0';

              parser->add(parser, parser->_start, parser->_row, parser->_column, false);

              ptr += parser->_separatorLength;
              parser->_column++;
              parser->_state = TRI_CSV_PARSER_BOF;
            }

            // found eol
            else {
              *qtr = '\0';

              parser->end(parser, parser->_start, parser->_row, parser->_column, false);

              ptr += parser->_eolLength;
              parser->_row++;
              parser->_state = TRI_CSV_PARSER_BOL;
            }
          }

          // need more input
          else {
            parser->_written = qtr;
            parser->_current = ptr;
            return TRI_ERROR_NO_ERROR;
          }

          break;

        case TRI_CSV_PARSER_WITHIN_QUOTED_FIELD:
          assert(parser->_useQuote);
          while (ptr < parser->_stop && *ptr != parser->_quote) {
            *qtr++ = *ptr++;
          }
          
          // found quote, need at least another quote, a separator, or a eol
          if (ptr + 1 < parser->_stop) {
            ++ptr;

            // a real quote
            if (*ptr == parser->_quote) {
              *qtr++ = parser->_quote;
              ptr++;
              break;
            }
            
            // ignore spaces
            while ((*ptr == ' ' || *ptr == '\t') && (ptr + 1) < parser->_stop) {
              ++ptr;
            }

            // found separator
            if (ptr + parser->_separatorLength < parser->_stop && memcmp(ptr, parser->_separator, parser->_separatorLength) == 0) {
              *qtr = '\0';

              parser->add(parser, parser->_start, parser->_row, parser->_column, true);

              ptr += parser->_separatorLength;
              parser->_column++;
              parser->_state = TRI_CSV_PARSER_BOF;
            }

            // found eol
            else if (strstr(ptr, parser->_eol) == ptr) {
              *qtr = '\0';

              parser->end(parser, parser->_start, parser->_row, parser->_column, true);

              ptr += parser->_eolLength;
              parser->_row++;
              parser->_state = TRI_CSV_PARSER_BOL;
            }
            
            // ups
            else {
              parser->_state = TRI_CSV_PARSER_CORRUPTED;
            }
          }

          // need more input
          else {
            parser->_written = qtr;
            parser->_current = ptr;
            return true;
          }

          break;
      }
    }
  }

  return TRI_ERROR_CORRUPTED_CSV;
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|// --SECTION--\\|/// @\\}\\)"
// End:
