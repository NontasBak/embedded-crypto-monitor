import { TokenIcon } from "@web3icons/react";
import {
    Sidebar,
    SidebarContent,
    SidebarGroup,
    SidebarGroupContent,
    SidebarGroupLabel,
    SidebarMenu,
    SidebarMenuButton,
    SidebarMenuItem,
    SidebarProvider,
} from "@/components/ui/sidebar";
import { Coins } from "lucide-react";

const SYMBOLS = [
    { title: "Btc", url: "#", icon: "btc" },
    { title: "Ada", url: "#", icon: "ada" },
    { title: "Eth", url: "#", icon: "eth" },
    { title: "Doge", url: "#", icon: "dog" },
    { title: "Xrp", url: "#", icon: "xrp" },
    { title: "Sol", url: "#", icon: "sol" },
    { title: "Ltc", url: "#", icon: "ltc" },
    { title: "Bnb", url: "#", icon: "bnb" },
];

function AppSidebar() {
    return (
        <SidebarProvider>
            <Sidebar>
                <SidebarContent>
                    <SidebarGroup className="flex flex-col gap-4">
                        <SidebarGroupLabel className="text-xl py-6 flex gap-4">
                            <Coins />
                            Crypto Monitor
                        </SidebarGroupLabel>
                        <SidebarGroupContent>
                            <SidebarMenu>
                                {SYMBOLS.map((symbol) => (
                                    <SidebarMenuItem key={symbol.title}>
                                        <SidebarMenuButton
                                            asChild
                                            size="lg"
                                            className="text-lg"
                                        >
                                            <a href={symbol.url}>
                                                <TokenIcon
                                                    symbol={symbol.icon}
                                                    variant="mono"
                                                    size="64"
                                                    className="icon"
                                                />
                                                <span>{symbol.title}</span>
                                            </a>
                                        </SidebarMenuButton>
                                    </SidebarMenuItem>
                                ))}
                            </SidebarMenu>
                        </SidebarGroupContent>
                    </SidebarGroup>
                </SidebarContent>
            </Sidebar>
        </SidebarProvider>
    );
}

export default AppSidebar;
