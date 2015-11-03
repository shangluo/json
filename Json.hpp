#ifndef _JSON_H_
#define _JSON_H_

#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include <memory>
#include <map>
#include <vector>
#include <utility>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <unordered_map>

namespace Json
{
    using namespace std;
    
    string readFile(string const &fileName)
    {
        string content;
        ifstream fIn;
        fIn.open(fileName);
        if (fIn.good())
        {
            content.assign(istreambuf_iterator<char>(fIn), istreambuf_iterator<char>());
        }
        fIn.close();
        
        return content;
    }
    
    void writeFile(string const &fileName, string const &content)
    {
        ofstream fOut;
        fOut.open(fileName);
        if (fOut.good())
        {
            fOut.write(content.c_str(), content.length());
        }
        fOut.close();
    }
    
    class ParserException
    {
    public:
        ParserException(string const &message)
        :_message(message)
        {
            
        }
        
        string getMessage() const
        {
            return _message;
        }
    private:
        string _message;
    };
    
    class ParsingContext
    {
    public:
        ParsingContext(string const &content)
        :_content(content)
        ,_pos(0)
        ,_row(0)
        ,_column(0)
        {
            
        }
        
        char peek() { return _content[_pos]; }
        bool next(int movement = 1)
        {
            if (_pos < _content.length() - 1)
            {
                _pos += movement;
                _column += movement;
                return true;
            }
            return false;
        }
        
        size_t getPos() const { return _pos; }
        
        void nextRow()
        {
            ++_row;
            _column = 0;
        }

        size_t row() const
        {
            return _row;
        }

        size_t column() const
        {
            return _column;
        }
        
    private:
        string _content;
        size_t _pos;
        size_t _row;
        size_t _column;
    };
    
    class Lexer
    {
    public:
        enum TOKEN_TYPE
        {
            JSON_OBJECT_START, // '{'
            JSON_OBJECT_END, // '}'
            JSON_ARRAY_START, // '['
            JSON_ARRAY_END, // ']'
            JSON_COLON, // ':'
            JSON_COMMA, // ','
            JSON_BOOLEAN, // true or false
            JSON_STRING, // string
            JSON_NUMBER, // numbric
            JSON_NULL // null
        };
        
        struct Token
        {
            TOKEN_TYPE type;
            string stringValue;
            union
            {
                //string stringValue;
                bool booleanValue;
                double numberValue;
            };
        };
        
        bool load(string const &fileName);
        
        const Token& peek() const
        {
            return *_itr;
        }
        
        bool next(int movement = 1)
        {
            if (_itr != _tokenArray.end() - 1)
            {
                _itr += movement;
                return true;
            }
            
            return false;
        }
        
    private:
        unique_ptr<ParsingContext> _parsingContext;
        vector<Token> _tokenArray;
        vector<Token>::iterator _itr;
    };
    
    bool Lexer::load(string const &content)
    {
        if (content.empty())
        {
            return false;
        }
        _parsingContext = make_unique<ParsingContext>(content);
        
        auto readString = [this]() -> string
        {
            string str;
            while (_parsingContext->next())
            {
                char ch = _parsingContext->peek();
                if (ch == '\\')
                {
                    if (_parsingContext->next())
                    {
                        char type = _parsingContext->peek();
                        if (type ==  'u')
                        {
                            // read four more
                            string strUnicode;
                            int chrIndex = 0;
                            while (chrIndex++ < 4 && _parsingContext->next())
                            {
                                strUnicode += _parsingContext->peek();
                            }

                            wchar_t wchr = 0;
                            wchr = stoi(strUnicode, 0, 16);
                            std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1;
                            str += conv1.to_bytes(wchr);
                        }
                        else
                        {
                            switch (type)
                            {
                            case '\"':
                            case '\\':
                            case '/':
                                break;
                            case 'b':
                                type = '\b';
                                break;
                            case 'f':
                                type = '\f';
                                break;
                            case 'n':
                                type = '\n';
                                break;
                            case 'r':
                                type = '\r';
                                break;
                            case 't':
                                type = '\t';
                                break;
                            default:
                                break;
                            }
                            
                            str += type;
                        }
                    }
                }
                else if (ch == '\"')
                {
                    //_parsingContext->next();
                    break;
                }
                else
                {
                    str += ch;
                }
            }
            
            return str;
        };
        
        auto readExpect = [this](const string expect)
        {
            const string pattern = expect;
            size_t chIndex = 0;
            do
            {
                char ch = _parsingContext->peek();
                if (ch != pattern[chIndex++])
                {
                    throw ParserException(string("error around charactor :") + ch + string{ " at pos (" } \
                        + to_string(_parsingContext->row()) + "," + to_string(_parsingContext->column()) + "), maybe it's " + expect + "?");
                }
            } while (_parsingContext->next() && chIndex < expect.length());
            
            _parsingContext->next(-1);
        };
        
        auto isNumberic = [](char ch)
        {
            return ch == '-' || ch == '+' || ch == 'e' || ch == '.' || isdigit(ch);
        };
        
        auto readNumber = [this, isNumberic]()-> double
        {
            string str;
            
            do
            {
                char ch = _parsingContext->peek();
                if (isNumberic(ch))
                {
                    str += ch;
                }
                else
                {
                    break;
                }
            } while (_parsingContext->next());
            
            _parsingContext->next(-1);
            
            try
            {
                return stod(str);
            }
            catch (invalid_argument const &i)
            {
                throw ParserException(string{"error at pos (" } + to_string(_parsingContext->row()) + "," + to_string(_parsingContext->column()) + "), " + i.what());
            }
        };
        
        do
        {
            Token token;
            char ch = _parsingContext->peek();
            
            if (isspace(ch))
            {
                if (ch == '\n')
                {
                    _parsingContext->nextRow();
                }
                continue;
            }
            
            switch (ch)
            {
                case '{':
                    token.type = TOKEN_TYPE::JSON_OBJECT_START;
                    break;
                case '}':
                    token.type = TOKEN_TYPE::JSON_OBJECT_END;
                    break;
                case '[':
                    token.type = TOKEN_TYPE::JSON_ARRAY_START;
                    break;
                case ']':
                    token.type = TOKEN_TYPE::JSON_ARRAY_END;
                    break;
                case '"':
                    token.type = TOKEN_TYPE::JSON_STRING;
                    token.stringValue = readString();
                    break;
                case ':':
                case ',':
                    token.type = ch == ':' ? TOKEN_TYPE::JSON_COLON : TOKEN_TYPE::JSON_COMMA;
                    break;
                case 't':
                case 'f':
                    token.type = TOKEN_TYPE::JSON_BOOLEAN;
                    token.booleanValue = ch == 't';
                    readExpect(ch == 't' ? "true" : "false");
                    break;
                case 'n':
                    token.type = TOKEN_TYPE::JSON_NULL;
                    readExpect("null");
                    break;
                    
                default:
                    if (isNumberic(ch))
                    {
                        token.type = TOKEN_TYPE::JSON_NUMBER;
                        token.numberValue = readNumber();
                    }
                    else
                    {
                        throw ParserException(string("error around charactor :") + ch + string{ " at pos (" } \
                            + to_string(_parsingContext->row()) + "," + to_string(_parsingContext->column()) + ")"); 
                    }
                    break;
            }
            
            _tokenArray.push_back(token);
        } while (_parsingContext->next());
        
        _itr = _tokenArray.begin();
        
        return _tokenArray.size() > 0;
    }
    
    enum VALUE_TYPE
    {
        JSON_OBJECT,
        JSON_ARRAY,
        JSON_NUMBER,
        JSON_STRING,
        JSON_BOOLEAN,
        JSON_NULL
    };
    
    class Value
    {
    public:
        typedef shared_ptr<Value> Ptr;
        typedef list<Value> List;
        
        Value(VALUE_TYPE type)
        :_type(type)
        {
            
        }
        
        virtual ~Value() {}
        virtual string toString() const = 0;
        
        VALUE_TYPE getType() const { return _type; }
    private:
        VALUE_TYPE _type;
    };
    
    class String : public Value
    {
    public:
        typedef shared_ptr<String> Ptr;
        typedef list<String> List;
        
        String(string const &value)
        :Value(VALUE_TYPE::JSON_STRING)
        ,_value(value)
        {
            
        }

        String(const char *chr)
            :Value(VALUE_TYPE::JSON_STRING)
            , _value(chr)
        {

        }
        
        string toSTDString() const { return _value; } // get utf8 string
        wstring toSTDWString() const // get unicode string
        { 
            wstring_convert<codecvt_utf8<wchar_t>> cvt;
            return cvt.from_bytes(_value);
        } 

        virtual string toString() const override 
        {
            auto escapeString = [this](const wstring &str) -> string
            {
                return accumulate(begin(str), end(str), string{}, [](string const &str, wchar_t ch) -> string
                {
                    if (iswascii(ch)) // don't encode ascii, reduce size
                    {
                        string strChr;

                        switch (ch)
                        {
                        case '\"':
                        case '\\':
                        case '/':
                            break;
                        case '\b':
                            strChr = "\b";
                            break;
                        case '\f':
                            strChr = "\f";
                            break;
                        case '\n':
                            strChr = "\f";
                            break;
                        case '\r':
                            strChr = "\r";
                            break;
                        case '\t':
                            strChr = "\t";
                            break;
                        default:
                            strChr = static_cast<char>(ch);
                            break;
                        }

                        return str + strChr;
                    }
                    
                    stringstream stream;
                    stream << setfill('0') << setw(4) << hex << (int)ch;
                    string result(stream.str());

                    return str + "\\u" + result;
                });
            };

            string rawStr = _value;
            try
            {
                auto wstr = toSTDWString();
                rawStr = escapeString(wstr);
            }
            catch (...)
            {
                // convert failed, maybe using native codec?
            }

            return '\"' + rawStr + '\"';
        }
        
        bool operator <(String const &rhs) const
        {
            return _value < rhs._value;
        }

	bool operator ==(String const &rhs) const
	{
		return _value == rhs._value;
	}

	struct hashFuc
	{
		size_t operator()(String const &rhs) const
		{
			return hash<string>()(rhs._value);
		}
	};

    private:
        string _value;
    };
    
    class Object : public Value
    {
    public:
        typedef shared_ptr<Object> Ptr;
        typedef list<Object> List;
        typedef unordered_map<String, Value::Ptr, String::hashFuc> ValueDict;
        
        Object()
        :Value(VALUE_TYPE::JSON_OBJECT)
        {
            
        }
        
        Value::Ptr &operator[](const String &key)
        {
            return _items[key];
        }
        
        template <class T>
        typename const T::Ptr get(const String &key) const
        {
            auto itr = _items.find(key);
            if (itr != _items.end())
            {
                return Json::ConvertJson<T>(itr->second);
            }
            
            return nullptr;
        }

        template <class T>
        typename T::Ptr get(const String &key)
        {
            auto itr = _items.find(key);
            if (itr != _items.end())
            {
                return Json::ConvertJson<T>(itr->second);
            }

            return nullptr;
        }
        
        size_t count() const { return _items.size(); }
        
        void insert(const String &key, Value::Ptr const &value)
        {
            _items[key] = value;
        }
        
        virtual string toString() const override
        {
            string str = "{";
            for (auto itr = _items.begin(); itr != _items.end(); )
            {
                str += (itr->first.toString() + ':' + itr->second->toString());
                
                if (++itr != _items.end())
                {
                    str += ',';
                }
            }
            
            return str + '}';
        }
        
    private:
        ValueDict _items;
    };
    
    class Array : public Value
    {
    public:
        typedef shared_ptr<Array> Ptr;
        typedef list<Array> List;
        typedef vector<Value::Ptr> ValueArray;
        
        Array()
        :Value(VALUE_TYPE::JSON_ARRAY)
        {
            
        }
        
        const Value::Ptr &operator[](size_t index) const
        {
            return _items[index];
        }
        
        Value::Ptr &operator[](size_t index)
        {
            return _items[index];
        }

        void push_back(Value::Ptr const &value)
        {
            _items.push_back(value);
        }
        
        size_t count() const { return _items.size(); }
        
        virtual string toString() const override
        {
            string str = "[";
            for (auto itr = _items.begin(); itr != _items.end(); )
            {
                str += (*itr)->toString();
                
                if (++itr != _items.end())
                {
                    str += ',';
                }
            }
            
            return str + ']';
        }
        
    private:
        ValueArray _items;
    };
    
    class Number : public Value
    {
    public:
        typedef shared_ptr<Number> Ptr;
        typedef list<Number> List;
        
    public:
        Number(double value)
        :Value(VALUE_TYPE::JSON_NUMBER)
        ,_value(value)
        {
            
        }
        
        long long toLongLong () const { return static_cast<long long>(_value); }
        long long toDouble () const { return _value; }
        
        virtual string toString() const override
        {
            return to_string(_value);
        }
        
    private:
        double _value;
    };
    
    class Boolean : public Value
    {
    public:
        typedef shared_ptr<Boolean> Ptr;
        typedef list<Boolean> List;
        
    public:
        Boolean(bool value)
        :Value(VALUE_TYPE::JSON_BOOLEAN)
        ,_value(value)
        {
            
        }
        
        bool toBool() const { return _value; }
        
        virtual string toString() const override
        {
            return _value ? "true" : "false";
        }
        
    private:
        bool _value;
    };
    
    class Null : public Value
    {
    public:
        typedef shared_ptr<Null> Ptr;
        typedef list<Null> List;
        
    public:
        Null()
        :Value(VALUE_TYPE::JSON_NULL)
        {
            
        }
        
        virtual string toString() const override
        {
            return "null";
        }
        
    };

    template<class T, class... _Types> inline
        shared_ptr<T> CreateJson(_Types&&... _Args)
    {	// make a shared_ptr
        auto ptr = make_shared<T>(forward<_Types>(_Args)...);
        return ptr;
    }


    // convert json value from one type to another
    template<class T1, class T2>
        shared_ptr<T1>
    ConvertJson(const shared_ptr<T2>& objPtr)
    {	
        return static_pointer_cast<T1, T2>(objPtr);
    }
    
    class FileReader
    {
    public:
        Value::Ptr parseFile(string const &fileName);
        Value::Ptr parse(string const &fileName);
        
    private:
        Value::Ptr readValue();
        
    private:
        Value::Ptr _object;
        unique_ptr<Lexer> _lexer;
    };
    
    Value::Ptr FileReader::parseFile(const string &fileName)
    {
        string content = readFile(fileName);
        return parse(content);
    }
    
    Value::Ptr FileReader::parse(const string &content)
    {
        _lexer = make_unique<Lexer>();
        if (_lexer->load(content))
        {
            _object = readValue();
        }
        else
        {
            _object = nullptr;
        }
        
        
        return _object;
    }
    
    Value::Ptr FileReader::readValue()
    {
        auto readExpect = [this](Lexer::TOKEN_TYPE type, bool peek = false)
        {
            auto token = _lexer->peek();
            if (token.type == type)
            {
                if (!peek)
                {
                    _lexer->next();
                }
                return true;
            }
            return false;
        };
        
        auto readObject = [this, readExpect]() -> Value::Ptr
        {
            Object::Ptr object = make_shared<Object>();
            readExpect(Lexer::TOKEN_TYPE::JSON_OBJECT_START);
            
            do
            {
                auto token = _lexer->peek();
                if (token.type == Lexer::TOKEN_TYPE::JSON_OBJECT_END)
                {
                    break;
                }
                if (_lexer->next())
                {
                    if (token.type == Lexer::TOKEN_TYPE::JSON_STRING)
                    {
                        auto key = String{ token.stringValue };
                        readExpect(Lexer::TOKEN_TYPE::JSON_COLON);
                        {
                            
                            auto value = readValue();
                            //(*object)[key] = v;
                            object->insert(key, value);
                            
                            if (_lexer->next() && !readExpect(Lexer::TOKEN_TYPE::JSON_COMMA, true))
                            {
                                break;
                            }
                        }
                    }
                }
            } while (_lexer->next());
            
            //readExpect(Lexer::TOKEN_TYPE::JSON_OBJECT_END);
            
            return static_pointer_cast<Value>(object);
        };
        
        auto readArray = [this, readExpect]() -> Value::Ptr
        {
            Array::Ptr array = make_shared<Array>();
            readExpect(Lexer::TOKEN_TYPE::JSON_ARRAY_START);
            
            do
            {
                Value::Ptr value = readValue();
                array->push_back(value);
                
                if (_lexer->peek().type == Lexer::TOKEN_TYPE::JSON_ARRAY_END)
                {
                    break;
                }
                
                if (_lexer->next() && !readExpect(Lexer::TOKEN_TYPE::JSON_COMMA, true))
                {
                    break;
                }
            } while (_lexer->next());
            
            //readExpect(Lexer::TOKEN_TYPE::JSON_ARRAY_END);
            
            return static_pointer_cast<Value>(array);
            
        };
        
        Value::Ptr value;
        auto token = _lexer->peek();
        switch (token.type)
        {
            case Lexer::TOKEN_TYPE::JSON_OBJECT_START:
                value = readObject();
                break;
                
            case Lexer::TOKEN_TYPE::JSON_ARRAY_START:
                value = readArray();
                break;
                
            case Lexer::TOKEN_TYPE::JSON_BOOLEAN:
                value = make_shared<Boolean>(token.booleanValue);
                break;
            case Lexer::TOKEN_TYPE::JSON_NULL:
                value = make_shared<Null>();
                break;
                
            case Lexer::TOKEN_TYPE::JSON_NUMBER:
                value = make_shared<Number>(token.numberValue);
                break;
                
            case Lexer::TOKEN_TYPE::JSON_STRING:
                value = make_shared<String>(token.stringValue);
                break;
                
            default:
                break;
        }
        
        //
        return value;
    }
    
    class FileWriter
    {
    public:
        void write(const string &fileName, const Value::Ptr &value);
    };
    
    void FileWriter::write(const string &fileName, const Value::Ptr &value)
    {
        writeFile(fileName, value->toString());
    }
}

#endif /* _JSON_H_ */
