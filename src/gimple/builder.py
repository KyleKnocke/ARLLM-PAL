"""
GIMPLE Code Generator

Converts typed AST to GIMPLE intermediate representation (three-address code).
Outputs C-like code that will be compiled by GCC for maximum optimization.
"""

from dataclasses import dataclass
from typing import List, Optional, Dict, Tuple, Set
from src.common.types import Type, INT_TYPE, BOOL_TYPE, REAL_TYPE, STRING_TYPE, FunctionT, ArrayT
from src.parser import ast_nodes as ast


# ============================================================================
# GIMPLE IR Data Structures
# ============================================================================

@dataclass
class GimpleStatement:
    """Base class for GIMPLE statements."""
    pass


@dataclass
class GimpleAssignment(GimpleStatement):
    """Assignment: var = expr."""
    target: str              # Variable name
    expression: str          # Expression (C syntax)
    type_: Type
    
    def __str__(self) -> str:
        return f"{self.target} = {self.expression};"


@dataclass
class GimpleCall(GimpleStatement):
    """Function call: result = func(args...)."""
    target: Optional[str]    # None if discarding return value
    func_name: str
    args: List[str]
    return_type: Optional[Type] = None
    
    def __str__(self) -> str:
        args_str = ", ".join(self.args)
        if self.target:
            return f"{self.target} = {self.func_name}({args_str});"
        else:
            return f"{self.func_name}({args_str});"


@dataclass
class GimpleLabel(GimpleStatement):
    """Label for control flow: label_name:."""
    name: str
    
    def __str__(self) -> str:
        return f"{self.name}:"


@dataclass
class GimpleBranch(GimpleStatement):
    """Unconditional branch: goto label."""
    target: str
    
    def __str__(self) -> str:
        return f"goto {self.target};"


@dataclass
class GimpleConditionalBranch(GimpleStatement):
    """Conditional branch: if (cond) goto label_true else goto label_false."""
    condition: str
    true_label: str
    false_label: str
    
    def __str__(self) -> str:
        return f"if ({self.condition}) goto {self.true_label} else goto {self.false_label};"


@dataclass
class GimpleReturn(GimpleStatement):
    """Return statement: return value."""
    value: Optional[str] = None
    
    def __str__(self) -> str:
        if self.value:
            return f"return {self.value};"
        return "return;"


@dataclass
class GimpleDeclaration(GimpleStatement):
    """Variable declaration: type_str name."""
    type_str: str
    name: str
    
    def __str__(self) -> str:
        return f"{self.type_str} {self.name};"


@dataclass
class BasicBlock:
    """A basic block (sequence of statements ending with terminator)."""
    label: str
    statements: List[GimpleStatement]
    
    def add_statement(self, stmt: GimpleStatement) -> None:
        self.statements.append(stmt)
    
    def __str__(self) -> str:
        lines = [f"{self.label}:"]
        for stmt in self.statements:
            lines.append(f"  {stmt}")
        return "\n".join(lines)


@dataclass
class GimpleFunction:
    """Complete GIMPLE function."""
    name: str
    params: List[Tuple[str, Type]]  # [(name, type), ...]
    return_type: Type
    blocks: List[BasicBlock]
    local_vars: Dict[str, Type]  # {name: type}
    
    def __str__(self) -> str:
        # Function signature
        param_strs = [f"{typ.to_c_type()} {name}" for name, typ in self.params]
        params_str = ", ".join(param_strs) if param_strs else "void"
        result = f"{self.return_type.to_c_type()} {self.name}({params_str})\n{{\n"
        
        # Local declarations
        for name, typ in self.local_vars.items():
            result += f"  {typ.to_c_type()} {name};\n"
        
        if self.local_vars:
            result += "\n"
        
        # Basic blocks
        for block in self.blocks:
            for line in str(block).split("\n"):
                result += f"  {line}\n"
        
        result += "}\n"
        return result


@dataclass
class GimpleModule:
    """Complete GIMPLE module (entire program)."""
    functions: Dict[str, GimpleFunction]
    globals: Dict[str, Type]  # Global variable declarations
    
    def __str__(self) -> str:
        result = "/* GIMPLE IR */\n\n"
        
        # Global declarations
        for name, typ in self.globals.items():
            result += f"{typ.to_c_type()} {name};\n"
        
        if self.globals:
            result += "\n"
        
        # Functions
        for func in self.functions.values():
            result += str(func)
            result += "\n"
        
        return result


# ============================================================================
# GIMPLE Generator
# ============================================================================

class GimpleGenerator:
    """Converts typed AST to GIMPLE IR."""
    
    def __init__(self):
        self.module = GimpleModule(functions={}, globals={})
        self.current_function: Optional[GimpleFunction] = None
        self.current_block: Optional[BasicBlock] = None
        self.label_counter = 0
        self.temp_counter = 0
        self.local_vars: Dict[str, Type] = {}
    
    def _gen_label(self, prefix: str = "label") -> str:
        """Generate a unique label."""
        self.label_counter += 1
        return f"{prefix}_{self.label_counter}"
    
    def _gen_temp(self, prefix: str = "t") -> str:
        """Generate a unique temporary variable."""
        self.temp_counter += 1
        return f"t_{self.temp_counter}"
    
    def _emit_statement(self, stmt: GimpleStatement) -> None:
        """Emit a GIMPLE statement to current block."""
        if self.current_block:
            self.current_block.add_statement(stmt)
    
    def _emit_assignment(self, var: str, expr: str, typ: Type) -> str:
        """Emit an assignment and return the target variable."""
        if var not in self.local_vars:
            self.local_vars[var] = typ
        self._emit_statement(GimpleAssignment(var, expr, typ))
        return var
    
    def generate(self, program: ast.Program) -> GimpleModule:
        """Generate GIMPLE for a complete program."""
        for defn in program.definitions:
            self._generate_function(defn)
        return self.module
    
    def _generate_function(self, defn: ast.Definition) -> None:
        """Generate GIMPLE for a function definition."""
        # Create function object
        param_types = []
        for param in defn.parameters:
            typ = param.type_annotation or INT_TYPE
            param_types.append((param.name, typ))
        
        return_type = defn.return_type or INT_TYPE
        func = GimpleFunction(
            name=defn.name,
            params=param_types,
            return_type=return_type,
            blocks=[],
            local_vars={}
        )
        
        self.current_function = func
        self.local_vars = {}
        self.temp_counter = 0
        self.label_counter = 0
        
        # Add entry block
        entry_block = BasicBlock(label="entry", statements=[])
        func.blocks.append(entry_block)
        self.current_block = entry_block
        
        # Generate code for body
        body_var = self._generate_expression(defn.body, return_type)
        self._emit_statement(GimpleReturn(body_var))
        
        # Finalize
        func.local_vars = self.local_vars
        self.module.functions[defn.name] = func
        self.current_function = None
    
    def _generate_expression(self, expr: ast.ASTNode, expected_type: Optional[Type] = None) -> str:
        """Generate code for an expression, return variable holding result."""
        
        if isinstance(expr, ast.Literal):
            return str(expr.value)
        
        elif isinstance(expr, ast.Variable):
            return expr.name
        
        elif isinstance(expr, ast.BinaryOp):
            left = self._generate_expression(expr.left)
            right = self._generate_expression(expr.right)
            
            # Map PAL operators to C operators
            op_map = {
                "+": "+", "-": "-", "×": "*", "÷": "/",
                "∧": "&&", "∨": "||", "¬": "!",
                "=": "==", "≠": "!=", "<": "<", "≤": "<=", ">": ">", "≥": ">=",
                "⊕": "^",  # XOR
            }
            c_op = op_map.get(expr.operator, expr.operator)
            
            result_var = self._gen_temp("binop")
            expr_str = f"{left} {c_op} {right}"
            return self._emit_assignment(result_var, expr_str, expected_type or INT_TYPE)
        
        elif isinstance(expr, ast.UnaryOp):
            operand = self._generate_expression(expr.operand)
            op_map = {"¬": "!", "-": "-"}
            c_op = op_map.get(expr.operator, expr.operator)
            
            result_var = self._gen_temp("unop")
            expr_str = f"{c_op}{operand}"
            return self._emit_assignment(result_var, expr_str, expected_type or INT_TYPE)
        
        elif isinstance(expr, ast.Conditional):
            # Generate conditional: if (cond) {...} else {...}
            true_label = self._gen_label("then")
            false_label = self._gen_label("else")
            merge_label = self._gen_label("merge")
            
            cond_var = self._generate_expression(expr.condition, BOOL_TYPE)
            self._emit_statement(GimpleConditionalBranch(cond_var, true_label, false_label))
            
            # True branch
            true_block = BasicBlock(label=true_label, statements=[])
            self.current_block = true_block
            self.current_function.blocks.append(true_block)
            true_var = self._generate_expression(expr.then_branch, expected_type)
            self._emit_statement(GimpleBranch(merge_label))
            
            # False branch
            false_block = BasicBlock(label=false_label, statements=[])
            self.current_block = false_block
            self.current_function.blocks.append(false_block)
            false_var = self._generate_expression(expr.else_branch, expected_type)
            self._emit_statement(GimpleBranch(merge_label))
            
            # Merge point
            merge_block = BasicBlock(label=merge_label, statements=[])
            self.current_block = merge_block
            self.current_function.blocks.append(merge_block)
            
            # Phi-like logic: create result variable
            result_var = self._gen_temp("cond_result")
            return result_var
        
        elif isinstance(expr, ast.Application):
            # Function call
            func_name = expr.func.name if isinstance(expr.func, ast.Variable) else "unknown_func"
            args = [self._generate_expression(arg) for arg in expr.args]
            
            result_var = self._gen_temp("call_result")
            return_type = expected_type or INT_TYPE
            self._emit_statement(GimpleCall(result_var, func_name, args, return_type))
            self.local_vars[result_var] = return_type
            return result_var
        
        elif isinstance(expr, ast.Lambda):
            # Lambdas are typically not directly translated to GIMPLE
            # They would be hoisted to function definitions
            # For now, return a placeholder
            return "lambda_placeholder"
        
        else:
            # Default: return 0
            return "0"
    
    def emit_to_string(self) -> str:
        """Emit complete GIMPLE IR as string."""
        return str(self.module)


# ============================================================================
# Helper function for code generation
# ============================================================================

def generate_gimple(program: ast.Program) -> str:
    """Convenience function: generate GIMPLE for a program."""
    generator = GimpleGenerator()
    generator.generate(program)
    return generator.emit_to_string()
