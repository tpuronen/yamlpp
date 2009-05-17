#include <CppSpec/CppSpec.h>
#include <boost/spirit.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>
#include <map>
#include <ctime>

using CppSpec::Specification;
using namespace boost::spirit;

typedef boost::function2<void, const char*, const char*> grammar_cb;

struct YamlGrammar : public grammar<YamlGrammar> {
    YamlGrammar(grammar_cb& identifier, grammar_cb& string_value, grammar_cb& num_value, grammar_cb& list_item) : identifier(identifier), string_value(string_value),
    num_value(num_value), list_item(list_item) {
    }

    template<class ScannerT>
    struct definition {
        rule<ScannerT> property_id;
        rule<ScannerT> string_value;
        rule<ScannerT> num_value;
        rule<ScannerT> property;
        rule<ScannerT> list_item;
        rule<ScannerT> yaml_line;
        rule<ScannerT> yaml_document;

        definition(const YamlGrammar& self) {
            property_id = lexeme_d[+alnum_p];
            string_value = lexeme_d[+alpha_p];
            num_value = real_p;
            property = property_id[self.identifier] >> ch_p(':') >> (num_value[self.num_value] | string_value[self.string_value]);
            list_item = ch_p('-') >> lexeme_d[*alnum_p][self.list_item] >> eol_p;
            yaml_line = list_item | property;
            yaml_document = *yaml_line;
        }

        const rule<ScannerT>& start() {return yaml_document;}
    };

    grammar_cb& identifier;
    grammar_cb& string_value;
    grammar_cb& num_value;
    grammar_cb& list_item;
};

class List {
public:
    List() : list() {}
    List(const List& that) : list(that.list) {}
    ~List() {std::cerr << "in ~List\n";}

    template<class T>
    T& valueAs(size_t index) {
        std::cerr << "List::valueAs, " << list.size() << "\n";
        boost::any& any = list[index];
        std::cerr << "type: " << any.type().name() << "\n";
        return boost::any_cast<T&>(list[index]);
    }

    void add(const boost::any& item) {
        std::cerr << "add\n";
        list.push_back(item);
    }

private:
    List& operator=(const List&);

private:
    std::vector<boost::any> list;
};

class Document {
public:
    Document() : values(), current_id() {}

    parse_info<> parse(const std::string& data) {
        grammar_cb id_f(bind(&Document::id, this, _1, _2));
        grammar_cb value_f(bind(&Document::value, this, _1, _2));
        grammar_cb num_value_f(bind(&Document::num_value, this, _1, _2));
        grammar_cb list_item_f(bind(&Document::list_item, this, _1, _2));
        YamlGrammar grammar(id_f, value_f, num_value_f, list_item_f);
        return boost::spirit::parse(data.c_str(), grammar >> eps_p, space_p);
    }

    template<class T>
    T valueAs(const std::string& key) {
        return boost::any_cast<T>(values[key]);
    }

    List& list() {
        for (std::map<std::string, boost::any>::iterator it = values.begin(); it != values.end(); it++) {
            if (it->second.type() == typeid(List)) {
                return boost::any_cast<List&>(it->second);
            }
        }
        throw std::string("List not found");
    }

private:
    void id(const char* start, const char* end) {
        std::cerr << "in id: " << std::string(start, end) << "\n";
        current_id = std::string(start, end);
    }

    void value(const char* start, const char* end) {
        values[current_id] = boost::any(std::string(start, end));
    }

    void num_value(const char* start, const char* end) {
        int value(atoi(std::string(start, end).c_str()));
        values[current_id] = boost::any(value);
    }

    void list_item(const char* start, const char* end) {
        List& list = getOrCreateList();
        list.add(boost::any(std::string(start, end)));
    }

    List& getOrCreateList() {
        boost::any& current = values[current_id];
        if (current.type() == typeid(List)) {
            return boost::any_cast<List&>(current);
        }
        std::stringstream stamp;
        stamp << "list-" << ::time(NULL);
        current_id = stamp.str();
        List list;
        std::cerr << "list is probably deleted here\n";
        values[current_id] = boost::any(list);
        std::cerr << "list is probably deleted here\n";
        return list;
    }

private:
    std::map<std::string, boost::any> values;
    std::string current_id;
};

class ScalarParserSpec : public Specification<Document, ScalarParserSpec> {
public:
    ScalarParserSpec() {
        REGISTER_BEHAVIOUR(ScalarParserSpec, canParseStringsFromMappings);
        REGISTER_BEHAVIOUR(ScalarParserSpec, canParseNumbersFromMappings);
    }

    Document* createContext() {
        Document* doc = new Document();
        std::stringstream input;
        input << "foo:bar" << std::endl << "baz:zyx" << std::endl << "count: 5";
        doc->parse(input.str());
        return doc;
    }

    void canParseStringsFromMappings() {
        specify(context().valueAs<std::string>("foo"), should.equal("bar"));
        specify(context().valueAs<std::string>("baz"), should.equal("zyx"));
    }

    void canParseNumbersFromMappings() {
        specify(context().valueAs<int>("count"), should.equal(5));
    }

} scalarParserSpec;

class ListParserSpec : public Specification<Document, ListParserSpec> {
public:
    ListParserSpec() {
        REGISTER_BEHAVIOUR(ListParserSpec, canParseList);
    }

    void canParseList() {
        std::stringstream input;
        input << "- first" << std::endl << "- second" << std::endl << "- third";
        std::cerr << "foobar\n";
        parse_info<> info = context().parse(input.str());
        List& list = context().list();

        specify(list.valueAs<std::string>(0), should.equal("first"));
//        specify(list.valueAs<std::string>(1), should.equal("second"));
//        specify(list.valueAs<std::string>(2), should.equal("third"));
    }
} listParserSpec;
