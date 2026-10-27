# Publicação de versões do XPS3

As versões instaláveis são publicadas em [GitHub Releases](https://github.com/LeonardoFigueiredo24/XPS3/releases).

## Conteúdo de cada versão

- `XPS3_<versão>_x64.appx`: aplicativo para Xbox Developer Mode.
- `Microsoft.UI.Xaml.2.8.appx`: dependência WinUI exigida pelo pacote.
- `XPS3.Development.cer`: certificado de desenvolvimento correspondente.
- `checksums.sha256`: hashes SHA-256 dos arquivos distribuídos.
- `INSTALL.md`: instruções de instalação e teste.
- Código-fonte em `.zip` e `.tar.gz`, gerado automaticamente pelo GitHub.

## Como uma atualização é publicada

1. A versão do manifesto e o teste estático são atualizados no mesmo commit.
2. Um arquivo `docs/releases/v<versão>.md` registra mudanças, limitações e testes.
3. A tag `v<versão>` inicia o workflow `XPS3 UWP release`.
4. O workflow compila, assina, valida os checksums e cria a página da versão com os arquivos.

O workflow só publica se a tag for igual à versão do manifesto e todos os arquivos forem gerados e validados. Uma alteração comum na branch `main` não cria uma versão instalável.

