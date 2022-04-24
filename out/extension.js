"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.deactivate = exports.activate = void 0;
// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
const vscode = require("vscode");
const { spawn } = require("child_process");
// this method is called when your extension is activated
// your extension is activated the very first time the command is executed
function activate(context) {
    // Use the console to output diagnostic information (console.log) and errors (console.error)
    // This line of code will only be executed once when your extension is activated
    console.log('Congratulations, your extension "notedown" is now active!');
    // The command has been defined in the package.json file
    // Now provide the implementation of the command with registerCommand
    // The commandId parameter must match the command field in package.json
    context.subscriptions.push(vscode.commands.registerCommand('notedown.helloWorld', () => {
        // The code you place here will be executed every time your command is executed
        // Display a message box to the user
        vscode.window.showInformationMessage('Hello World from Notedown!');
    }));
    let currentPanel = undefined;
    let currentFile = undefined;
    let terminal = undefined;
    let extOutput = vscode.window.createOutputChannel("Notedown Compiler");
    const doUpdateWebView = () => {
        if (!currentPanel || !currentFile) {
            return;
        }
        let html = `
		<!DOCTYPE html>
		<html lang="en">
			<head>
				<meta charset="UTF-8">
				<meta name="viewport" content="width=device-width, initial-scale=1.0">
				<title>Cat Coding</title>
			</head>
			<body>
				${currentFile?.getText()}
			</body>
		</html>
		`;
        currentPanel.webview.html = html;
        if (!terminal) {
            terminal = vscode.window.createTerminal({
                name: "notedown-compiler",
                message: "This console is for use of the notedown extension. It is used to compile .nd files to .html"
            });
        }
        let tmpFile = "temporary";
        let tmpOut = "tempOut";
        terminal.sendText(`notedown -sd "${tmpFile}" -o "${tmpOut}`);
    };
    let updateTimeout = undefined;
    const updateWebView = () => {
        if (updateTimeout) {
            clearTimeout(updateTimeout);
        }
        updateTimeout = setTimeout(doUpdateWebView, 2000);
    };
    vscode.workspace.onDidChangeTextDocument((e) => {
        if (!e.document && e.document !== currentFile) {
            return;
        }
        if (currentPanel && currentFile) {
            updateWebView();
        }
    });
    context.subscriptions.push(vscode.commands.registerCommand('notedown.openPreview', () => {
        if (currentPanel && vscode.window.activeTextEditor?.document.uri === currentFile?.uri) {
            currentPanel.reveal();
        }
        else {
            currentPanel?.dispose();
            currentFile = vscode.window.activeTextEditor?.document;
            currentPanel = vscode.window.createWebviewPanel("notedownPreview", currentFile
                ? currentFile.fileName + " - Preview"
                : "Notedown Preview", vscode.ViewColumn.Two, {});
            currentPanel.onDidDispose(() => {
                currentPanel = undefined;
                currentFile = undefined;
            }, null, context.subscriptions);
            doUpdateWebView();
        }
        vscode.window.showInformationMessage(`Preview started for ${currentFile?.fileName}`);
    }));
    context.subscriptions.push(vscode.commands.registerCommand('notedown.execHelp', () => {
        let result = spawn("notedown", ["-h"], {
            timeout: 2000
        });
        result.on("error", (err) => {
            vscode.window.showErrorMessage("Notedown Compiler Error: " + err.message);
        });
        extOutput.show();
        result.stdout.on("data", (data) => {
            extOutput.append(data.toString());
        });
    }));
}
exports.activate = activate;
// this method is called when your extension is deactivated
function deactivate() { }
exports.deactivate = deactivate;
//# sourceMappingURL=extension.js.map