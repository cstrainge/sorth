
import * as path from 'path';
import * as vscode from 'vscode';
import * as lsp from 'vscode-languageclient/node';


let client: lsp.LanguageClient;


export function activate(context: vscode.ExtensionContext)
{
	console.log('Congratulations, your extension "strange-forth" is now active!');

	let disposable = vscode.commands.registerCommand('strange-forth.helloWorld',
        () =>
        {
		    vscode.window.showInformationMessage('Hello World from Strange Forth language!');
	    });

    let disposable2 = vscode.commands.registerCommand('strange-forth.funkTown',
        () =>
        {
            vscode.window.showInformationMessage("Client is running? " + client.isRunning() + ".");
        });


    const currentPlatform = process.platform;
    const currentArch = process.arch;

	context.subscriptions.push(disposable);
    context.subscriptions.push(disposable2);


    // const sorthExe = context.asAbsolutePath(path.join("..", "sorth_lsp"));
    const sorthExe = context.asAbsolutePath("sorth_lsp");


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

    console.log("Exe: " + sorthExe);

    const clientOptions: lsp.LanguageClientOptions = {
            documentSelector: [ { scheme: 'file', language: 'strangeforth' } ]
        };

    client = new lsp.LanguageClient('StrangeForthServer',
                                    'Strange Forth',
                                    serverOptions,
                                    clientOptions,
                                    true);

    client.start();

    console.log(client.isRunning());
}



export function deactivate(): Thenable<void> | undefined {
    if (!client)
    {
        return undefined;
    }

    console.log(client.isRunning());
    return client.stop();
}
