@page generator String Generators

# String Generators
String generators are used to generate input strings, e.g., via the command
line (see @ref driver) or for unit tests. They are based on the
@ref tdc::Generator class, implementing the @ref tdc::Generator::generate
function.

In order to allow for convenient use within tudocomp, the actual generation
should be implemented in a static function that receives all parameters directly
instead of relying on an algorithm configuration. The following class fully
implements a string generator that simply repeats a character a given number
of times:

@code{.cpp}
class MyGenerator : public Generator {
public:
    inline static Meta meta() {
        Meta m(Generator::type_desc(), "my_generator", "An example generator");
        m.param("length").primitive();
        m.param("char").primitive('a');
        return m;
    }

    inline static std::string generate(char c, size_t num) {
        return std::string(num, c);
    }

    using Generator::Generator;

    inline virtual std::string generate() override {
        return generate(
            (char)config().param("char").as_int(),
            config().param("length").as_int);
    }
};
@endcode

## Available String Generators
The following string generators are currently implemented in tudocomp:
* Random strings with uniform character distribution
  (@ref tdc::RandomUniformGenerator)
* Fibonacci words (@ref tdc::FibonacciGenerator)
* Thue-Morse strings (@ref tdc::ThueMorseGenerator)
* Run-Rich strings (@ref tdc::RunRichGenerator)

A full list can be found in the inheritance diagram in the API reference of
@ref tdc::Generator.
