
import * as path from 'path';
import * as fs from 'fs';
import * as vscode from 'vscode';
import * as lsp from 'vscode-languageclient/node';


let client: lsp.LanguageClient;


function extension(platform: string): string
{
    if (platform === "win32")
    {
        return ".bat";
    }

    return "";
}


export function activate(context: vscode.ExtensionContext)
{
	console.log('Strange Forth language service is now active!');

    const currentPlatform = process.platform;
    const currentArch = process.arch;

    console.log(`Detected platform: ${currentPlatform}/${currentArch}.`);


    const sorthDevExe = context.asAbsolutePath(path.join("..",
                                                         "sorth_lsp" + extension(currentPlatform)));

    const sorthExe = fs.existsSync(sorthDevExe)
        ? sorthDevExe
        : context.asAbsolutePath("sorth_lsp" + extension(currentPlatform));


    console.log(`Launching language server: ${sorthExe}.`);

    const server: lsp.Executable = {
            command: sorthExe,
            args: [ sorthExe, currentPlatform, currentArch ],
            transport: lsp.TransportKind.pipe,
            options: {
                cwd: context.asAbsolutePath("."),
                detached: false,
                shell: true
            }
        };

    const serverOptions: lsp.ServerOptions = {
            run: server,
            debug: server
        };


    const clientOptions: lsp.LanguageClientOptions = {
            documentSelector: [ { scheme: 'file', language: 'strangeforth' } ]
        };


    client = new lsp.LanguageClient('StrangeForthServer',
                                    'Strange Forth',
                                    serverOptions,
                                    clientOptions,
                                    true);

    client.start();
}



export function deactivate(): Thenable<void> | undefined {
    if (!client)
    {
        return undefined;
    }

    console.log(client.isRunning());
    return client.stop();
}
