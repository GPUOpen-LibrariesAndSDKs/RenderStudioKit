#include "Certificates.h"

#pragma warning(push, 0)
#ifdef _WIN32
#include <wincrypt.h>
#elif __linux__
#error "Linux certificate loading not implemented"
#endif
#pragma warning(pop)

namespace RenderStudio::Networking
{

void
AddWindowsRootCertificatesFromStores(boost::asio::ssl::context& ctx, const std::vector<std::string>& storeNames)
{
    X509_STORE* store = X509_STORE_new();

    for (const std::string& storeName : storeNames)
    {
        HCERTSTORE hStore = CertOpenSystemStore(0, storeName.c_str());
        if (hStore == NULL)
        {
            return;
        }

        PCCERT_CONTEXT pContext = NULL;
        while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL)
        {
            // convert from DER to internal format
            X509* x509 = d2i_X509(NULL, (const unsigned char**)&pContext->pbCertEncoded, pContext->cbCertEncoded);
            if (x509 != NULL)
            {
                X509_STORE_add_cert(store, x509);
                X509_free(x509);
            }
        }

        CertFreeCertificateContext(pContext);
        CertCloseStore(hStore, 0);
    }

    // attach X509_STORE to boost ssl context
    SSL_CTX_set_cert_store(ctx.native_handle(), store);
}

void
AddWindowsRootCertificates(boost::asio::ssl::context& ctx)
{
    AddWindowsRootCertificatesFromStores(ctx, { "ROOT", "CA" });
}

} // namespace RenderStudio::Networking