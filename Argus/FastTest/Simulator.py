from numba import types
from numba import njit, jit
from numba.extending import typeof_impl
from numba.extending import type_callable
from numba.extending import models, register_model
from numba.extending import overload_classmethod, overload_method
from numba.extending import lower_builtin
from numba.core import cgutils
from numba.extending import unbox, NativeValue
from numba.extending import box

import FastTest
from FastTest import Hydra

class HydraType(types.Type):
    def __init__(self):
        super(HydraType, self).__init__(name='Hydra')
        
    def test_numba_x():
        return 3.3
        
hydra_type = HydraType()

@typeof_impl.register(Hydra)
def typeof_index(val, c):
    return hydra_type

@type_callable(Hydra)
def type_hydra(context):
    def typer(logging):
        if isinstance(logging, types.intc):
            return hydra_type
    return typer

@register_model(HydraType)
class HydraModel(models.StructModel):
    def __init__(self, dmm, fe_type):
        members = [
            ('logging', types.int32)
            ]
        models.StructModel.__init__(self, dmm, fe_type, members)
               
@lower_builtin(Hydra, types.int32)
def impl_interval(context, builder, sig, args):
    typ = sig.return_type
    logging = args
    hydra = cgutils.create_struct_proxy(typ)(context, builder)
    hydra.logging = logging
    return hydra._getvalue()

@unbox(HydraType)
def unbox_interval(typ, obj, c):
    """
    Convert a Interval object to a native interval structure.
    """
    hydra = cgutils.create_struct_proxy(typ)(c.context, c.builder)
    is_error = cgutils.is_not_null(c.builder, c.pyapi.err_occurred())
    return NativeValue(hydra._getvalue(), is_error=is_error)

@box(HydraType)
def box_interval(typ, val, c):
    """
    Convert a native interval structure to an Interval object.
    """
    hydra = cgutils.create_struct_proxy(typ)(c.context, c.builder, value=val)
    class_obj = c.pyapi.unserialize(c.pyapi.serialize_object(hydra))
    res = c.pyapi.call_function_objargs(class_obj, (hydra.logging))
    c.pyapi.decref(hydra.logging)
    c.pyapi.decref(class_obj)
    return res


@overload_method(HydraType, "width")
def hydra_test(hydra):
    if isinstance(hydra, HydraType):
        def take_impl(hydra):
            return 3.2
        return take_impl

@njit()
def hydra_test(hydra):
    return hydra.width()

if __name__ == "__main__":
    hydra = FastTest.new_hydra(1)
    x = hydra_test(hydra)
    print(x)
    print(1)