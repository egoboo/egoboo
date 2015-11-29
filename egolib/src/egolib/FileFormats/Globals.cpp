#include "egolib/FileFormats/Globals.hpp"
#include "egolib/strutil.h"

tile_dictionary_t tile_dict;
treasure_table_t treasureTableList[MAX_TABLES];
wawalite_data_t wawalite_data;

/// Find the first treasure table of the specified name.
treasure_table_t *find_treasure_table(const std::string& name) {
    for (size_t i = 0; i < MAX_TABLES; i++) {
        if (treasureTableList[i].name == name) {
            return &(treasureTableList[i]);
        }
    }
    return nullptr;
}

bool get_random_treasure(const std::string& treasureTableName, std::string& treasureName)
{
	// The empty string is not a valid treasure table name and treasure table names must start with '%'.
    if (treasureTableName.empty() || '%' != treasureTableName[0]) {
        return false;
    }
    // To detect circular references, we keep the names of visited tables in this set.
    std::unordered_set<std::string> visited;
    // Make a local copy of the table name.
    std::string n = treasureTableName;

    bool found = false;
	// Keep searching until we have found something or abort searching for other reasons.
    while (!found) {
        // Check if this table name was already visited.
        auto it = visited.find(n);
        // If this table name was already visted ...
        if (it != visited.cend())
        {
            // ... stop searching.
            break;
        }
        // Mark this table name as visited.
        visited.insert(n);
        // Find the table for the name.
        treasure_table_t *t = find_treasure_table(n);
        // If the table does not exist or is empty ...
        if (nullptr == t && 0 == t->size) {
            // ... stop searching.
            break;
        } else {
            // Pick a random index number between 0 and size - 1 of the table and get the element at this index.
            int i = generate_irand_pair(IPair(0, t->size-1));
            n = t->entries[i];
            // If this is not a reference to yet another treasure table ...
            if ('%' != n[0]) {
                // ... an entry was found.
                found = true;
            }
            // Otherwise we keep searching.
        }
    }
    if (found) {
        treasureName = n;
        return true;
    } else {
        treasureName = std::string();
        return false;
    }
}

namespace TreasureTable {
struct Token {
    enum class Type {
        StartOfInput,
        EndOfInput,
        Reference,
        Name,
        EndOfTable,
        StartOfTable,
    };
    Type type;
    std::string text;
    Token(Type type, const std::string& text) : type(type), text(text) {}
};

struct Scanner
{
private:
    Token readElement(ReadContext& ctxt) {
        std::string text;
        if (ctxt.is('%'))
        {
            ctxt.next();
            text = '%' + ctxt.readName();
            return Token(Token::Type::Reference, text);
        }
        else
        {
            text = ctxt.readName();
            if (text == "END")
            {
                return Token(Token::Type::EndOfTable, text);
            }
            else
            {
                return Token(Token::Type::Name, text);
            }
        }
    }

public:
    std::vector<Token> read(ReadContext& ctxt)
    {
        std::vector<Token> tks;
        Token tk = Token(Token::Type::StartOfInput, "<start of input>");
        tks.push_back(tk);
        while (ctxt.skipToColon(true))
        {
            std::string name = ctxt.readName();
            tk = Token(Token::Type::StartOfTable, "%" + name);
            tks.push_back(tk);
            while (ctxt.skipToColon(false))
            {
                ctxt.skipWhiteSpaces();
                tk = readElement(ctxt);
                tks.push_back(tk);
                if (tk.type == Token::Type::EndOfTable)
                {
                    break;
                }
            }
        }
        tk = Token(Token::Type::EndOfInput, "<end of input>");
        tks.push_back(tk);
        return tks;
    }
};

struct Parser {

private:
    treasure_table_t *table;
    std::vector<Token>::const_iterator current, begin, end;
    size_t index;
private:
    void next()
    {
        ++current;
    }
    bool is(Token::Type type)
    {
        if (current == end) throw std::runtime_error("<internal error>");
        return (type == (*current).type);
    }
    // @code
    // file = startOfInput table* endOfInput
    // table = startOfTable (reference | name)* endOfTable
    // @endcode
    void parse(const std::vector<Token>& tks)
    {
        assert(!tks.empty()); // The token list is never empty.
        index = 0;
        begin = tks.cbegin(); end = tks.cend(); current = begin;

        // `<start of input>`
        if (!is(Token::Type::StartOfInput))
        {
            throw std::runtime_error("syntax error");
        }
        next();


        while (is(Token::Type::StartOfTable))
        {
            // action {
            if (index >= MAX_TABLES)
            {
                Log::Entry e(Log::Level::Warning, __FILE__, __LINE__, __FUNCTION__);
                e << "unable load random treasure table `" << current->text << "`. Only support " << MAX_TABLES << " random treasure tables are supported."
                  << Log::EndOfEntry;
                Log::get() << e;
                throw std::runtime_error(e.getText());
            }
            auto& table = treasureTableList[index++]; table.name = current->text;
            // }
            next();
            // `(reference>|name)*`
            while (is(Token::Type::Reference) || is(Token::Type::Name))
            {
                // action {
                table.add(current->text);
                // } 
                next();
            }
            // `endOfTable`
            if (!is(Token::Type::EndOfTable))
            {
                throw std::runtime_error("expected <end of table>");
            }
            next();
        }
        // `endOfInput`
        if (!is(Token::Type::EndOfInput))
        {
            throw std::runtime_error("expected <end of input>");
        }
    }

public:
    void parse(const std::string& filename)
    {
        // Try to open a context.
        ReadContext ctxt(filename);
        if (!ctxt.ensureOpen())
        {
            Log::Entry e(Log::Level::Error, __FILE__, __LINE__, __FUNCTION__);
            e << "unable to load random treasure tables file `" << filename << "`" << Log::EndOfEntry;
            Log::get() << e;
            throw std::runtime_error(e.getText());
        }
        Scanner scanner;
        parse(scanner.read(ctxt));
    }
};

}

void init_random_treasure_tables_vfs(const std::string& filepath)
{
    TreasureTable::Parser parser;
    parser.parse(filepath);
}
