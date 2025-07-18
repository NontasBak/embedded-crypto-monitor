import AppSidebar from "./Sidebar";
import Content from "./Content";
import { ThemeProvider } from "@/components/theme-provider";
import { ModeToggle } from "./components/mode-toggle";

function App() {
    return (
        <ThemeProvider defaultTheme="dark" storageKey="vite-ui-theme">
            <main className="flex gap-2">
                <AppSidebar />
                <Content />
                <ModeToggle />
            </main>
        </ThemeProvider>
    );
}

export default App;
