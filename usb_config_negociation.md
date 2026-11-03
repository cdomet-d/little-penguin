# What happens when you plug in a USB device ?

A whole bunch of stuff happens under the hood when you plug in a USB device. In the little pinguin challenge, we focus on the whole "attaching a driver" part, but as you probably guessed, that happens at the very end of the process.

Said process is described in detail in [section 9.1 of the USB 3.2 specification](./.assets/USB%203.2%20Revision%201.1.pdf). Below is an (AI generated, human annotated) sequence diagram of the process.

```mermaid
sequenceDiagram
    autonumber
    participant H as USB Host
    participant Hub as Hub / Root Hub
    participant D as USB Device

    Note over D: Not Attached

    D->>Hub: Physical attach
    Hub->>H: Port connect / status change
    Note over D: Attached

    Hub->>D: Apply VBUS / power
    Note over D: Powered

    Hub->>D: Port reset
    Note over D: Default state<br/>responds at the default address (0).

    Note over H: The host requests the device<br>descriptor to begin configuration.
    H->>D: GET_DESCRIPTOR(Device):
    D-->>H: Device descriptor<br/>(max packet size, class, vendor/product IDs)

    H->>D: SET_ADDRESS(Address)
    D-->>H: Status (success)
    Note over D: Address state<br/>unique device address assigned

    Note over H: At this point,<br/>the host knows what is connected.<br/>It request further information, such as the device's interfaces.
    H->>D: GET_DESCRIPTOR(Configuration, index 0)
    D-->>H: Configuration descriptor + interfaces + endpoints

    H->>D: GET_DESCRIPTOR(String) [optional]
    D-->>H: String descriptors

    H->>D: SET_CONFIGURATION(ConfigurationValue)
    D-->>H: Status (success)
    Note over D: Configured state<br/>function available for use

    H->>D: Normal transfers / class-specific requests
    D-->>H: Data / status

    alt No bus activity for suspend interval
        Note over D: Suspended state<br/>address and configuration retained
        H->>D: Bus resume / remote wakeup
        Note over D: Returns to prior active state
    end

    alt Configuration change
        H->>D: SET_CONFIGURATION(0 or new value)
        D-->>H: Status
        Note over D: Deconfigured or reconfigured
    end

    alt Detach, power loss, or port error
        Hub->>D: Remove power / disconnect
        Note over D: Not Attached
    end
```
