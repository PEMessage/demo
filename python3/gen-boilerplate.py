#!/usr/bin/env python3
import argparse
from abc import ABC, abstractmethod

class Template(ABC):
    @classmethod
    @abstractmethod
    def add_arguments(cls, subparser):
        pass
    
    @classmethod
    @abstractmethod
    def generate(cls, args):
        pass

class JavaMapClassTemplate(Template):
    """Generates a Java class that uses a Map to store fields"""
    
    @classmethod
    def add_arguments(cls, subparser):
        subparser.add_argument('--class', dest='class_name', default='A',
                             help='Name of the class to generate')
        subparser.add_argument('--fields', nargs='+', required=True,
                             type=cls.parse_field,
                             help='Fields in format "FieldName:Type" (e.g., "Age:int")')
    
    @staticmethod
    def parse_field(field_arg):
        try:
            name, type_ = field_arg.split(':')
            return (name, type_)
        except ValueError:
            raise argparse.ArgumentTypeError(f"Field '{field_arg}' should be in format 'name:type'")
    
    @classmethod
    def generate(cls, args):
        print(f"public class {args.class_name} {{")
        print("    private Map<String, Object> data;\n")
        print(f"    public {args.class_name}() {{")
        print("        data = new HashMap<>();")
        print("    }\n")

        for field, T in args.fields:
            param = field[0].lower() + field[1:]
            print(f"    public void set{field}({T} {param}) {{")
            print(f'        data.put("{field}", {param});')
            print("    }\n")

            print(f"    public {T} get{field}() {{")
            print(f'        return ({T}) data.get("{field}");')  # Added cast for type safety
            print("    }\n")

        print('    public Object getValueFromData(String key) {')
        print('        if(data.containsKey(key)){')
        print('            return data.get(key);')
        print('        }')
        print('        return null;')
        print('    }')
        print("}")

class JavaPojoTemplate(Template):
    """Generates a standard Java POJO with private fields and getters/setters"""
    
    @classmethod
    def add_arguments(cls, subparser):
        subparser.add_argument('--class', dest='class_name', default='A',
                             help='Name of the class to generate')
        subparser.add_argument('--fields', nargs='+', required=True,
                             type=cls.parse_field,
                             help='Fields in format "FieldName:Type" (e.g., "Age:int")')
    
    @staticmethod
    def parse_field(field_arg):
        try:
            name, type_ = field_arg.split(':')
            return (name, type_)
        except ValueError:
            raise argparse.ArgumentTypeError(f"Field '{field_arg}' should be in format 'name:type'")
    
    @classmethod
    def generate(cls, args):
        print(f"public class {args.class_name} {{")
        
        # Generate fields
        for field, T in args.fields:
            print(f"    private {T} {field[0].lower() + field[1:]};")
        print()
        
        # Generate constructor
        params = []
        assignments = []
        for field, T in args.fields:
            param_name = field[0].lower() + field[1:]
            params.append(f"{T} {param_name}")
            assignments.append(f"        this.{param_name} = {param_name};")
        
        print(f"    public {args.class_name}({', '.join(params)}) {{")
        print('\n'.join(assignments))
        print("    }\n")
        
        # Generate getters and setters
        for field, T in args.fields:
            param_name = field[0].lower() + field[1:]
            func_param_name = field[0].upper() + field[1:]
            print(f"    public {T} get{func_param_name}() {{")
            print(f"        return this.{param_name};")
            print("    }\n")
            
            print(f"    public void set{func_param_name}({T} {param_name}) {{")
            print(f"        this.{param_name} = {param_name};")
            print("    }\n")
        
        print("}")

ALLTEMPLATE = {
    'java-map': JavaMapClassTemplate,
    'java-pojo': JavaPojoTemplate
}
    

def main():
    parser = argparse.ArgumentParser(description='Code generator for various templates.')
    subparsers = parser.add_subparsers(dest='template', required=True, 
                                     help='Template to use for code generation')
    
    # Register all available templates
    for template_name, template_class in ALLTEMPLATE.items():
        subparser = subparsers.add_parser(template_name, help=template_class.__doc__)
        template_class.add_arguments(subparser)
        subparser.set_defaults(template_class=template_class)
    
    args = parser.parse_args()
    
    if not hasattr(args, 'template_class'):
        parser.print_help()
        return
    
    args.template_class.generate(args)

if __name__ == "__main__":
    main()
