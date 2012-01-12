/*****************************************************************************
  chromsweep.cpp

  (c) 2009 - Aaron Quinlan
  Hall Laboratory
  Department of Biochemistry and Molecular Genetics
  University of Virginia
  aaronquinlan@gmail.com

  Licenced under the GNU General Public License 2.0 license.
******************************************************************************/
#include "lineFileUtilities.h"
#include "chromsweep.h"
#include <queue>

bool after(const BED &a, const BED &b);

/*
    // constructor using existing BedFile pointers
*/
ChromSweep::ChromSweep(BedFile *query, BedFile *db, bool sameStrand, bool diffStrand, bool printHeader)
: _query(query)
, _db(db)
, _sameStrand(sameStrand)
, _diffStrand(diffStrand)
{
    _hits.reserve(100000);
    _cache.reserve(100000);
    
    _query->Open();
    if (printHeader) _query->PrintHeader();
    _db->Open();
    
    _query->GetNextBed(_curr_qy);
    _db->GetNextBed(_curr_db);
}

/*
    Constructor with filenames
*/
ChromSweep::ChromSweep(string &queryFile, string &dbFile) 
{
    _hits.reserve(100000);
    _cache.reserve(100000);
    
    _query = new BedFile(queryFile);
    _db = new BedFile(dbFile);
    
    _query->Open();
    _db->Open();
    
    _query->GetNextBed(_curr_qy);
    _db->GetNextBed(_curr_db);
}


/*
    Destructor
*/
ChromSweep::~ChromSweep(void) {
}


void ChromSweep::ScanCache() {
    vector<BED>::iterator c = _cache.begin();
    while (c != _cache.end())
    {
        if ((_curr_qy.chrom == c->chrom) && !(after(_curr_qy, *c))) {
            if (IsValidHit(_curr_qy, *c)) {
                _hits.push_back(*c);
            }
            ++c;
        }
        else {
            c = _cache.erase(c);
        }
    }
}


bool ChromSweep::ChromChange()
{
    // the files are on the same chrom
    if (_curr_qy.chrom == _curr_db.chrom) {
        return false;
    }
    // the query is ahead of the database. fast-forward the database to catch-up.
    else if (_curr_qy.chrom > _curr_db.chrom) {
        while (_db->GetNextBed(_curr_db, true) && _curr_db.chrom < _curr_qy.chrom)
        {
        }
        _cache.clear();
        return false;
    }
    // the database is ahead of the query.
    else {
        // 1. scan the cache for remaining hits on the query's current chrom.
        if (_curr_qy.chrom == _curr_chrom)
        {
            ScanCache();
            _results.push(make_pair(_curr_qy, _hits));
            _hits.clear();
        }
        // 2. fast-forward until we catch up and report 0 hits until we do.
        else if (_curr_qy.chrom < _curr_db.chrom)
        {
            _results.push(make_pair(_curr_qy, _no_hits));
            _cache.clear();
        }
        _query->GetNextBed(_curr_qy, true);
        _curr_chrom = _curr_qy.chrom;
        return true;
    }
}


bool ChromSweep::IsValidHit(const BED &query, const BED &db) {
    // do we have an overlap in the DB?
    if (overlaps(query.start, query.end, db.start, db.end) > 0) {
        // Now test for necessary strandedness.
        bool strands_are_same = (query.strand == db.strand);
        if ( (_sameStrand == false && _diffStrand == false)
             ||
             (_sameStrand == true && strands_are_same == true)
             ||
             (_diffStrand == true && strands_are_same == false)
           )
        {
            return true;
        }
    }
    return false;
}


bool ChromSweep::Next(pair<BED, vector<BED> > &next) {
    if (!_query->Empty()) {
        // have we changed chromosomes?
        if (ChromChange() == false) {
            // scan the database cache for hits
            ScanCache();
            // advance the db until we are ahead of the query. update hits and cache as necessary
            while (!_db->Empty() && 
                   _curr_qy.chrom == _curr_db.chrom && 
                   !(after(_curr_db, _curr_qy)))
            {
                if (IsValidHit(_curr_qy, _curr_db)) {
                    _hits.push_back(_curr_db);
                }
                _cache.push_back(_curr_db);
                _db->GetNextBed(_curr_db, true);
            }
            // add the hits for this query to the pump
            _results.push(make_pair(_curr_qy, _hits));
            // reset for the next query
            _hits.clear();
            _query->GetNextBed(_curr_qy, true);
            _curr_chrom = _curr_qy.chrom;
        }
    }
    // report the next set if hits if there are still overlaps in the pump
    if (!_results.empty()) {
        next = _results.front();
        _results.pop();
        return true;
    }
    else {
        return false;
    }
}
