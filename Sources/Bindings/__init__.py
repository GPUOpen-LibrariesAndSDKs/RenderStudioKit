from . import _WebUsdAssetResolver
from pxr import Tf
Tf.PrepareModule(_WebUsdAssetResolver, locals())
del Tf

try:
    import __DOC
    __DOC.Execute(locals())
    del __DOC
except Exception:
    try:
        import __tmpDoc
        __tmpDoc.Execute(locals())
        del __tmpDoc
    except:
        pass
