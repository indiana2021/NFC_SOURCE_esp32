graph TD
    A[Start] --> B{setup()};
    B --> C{loop()};

    subgraph MainLoop
        C --> D{handleInput()};
        D --> E{switch (currentMenu)};
    end

    E --> F[MAIN_MENU];
    E --> G[READ_CARD];
    E --> H[WRITE_CARD];
    E --> I[EMULATE_CARD];
    E --> J[BRUTE_FORCE];
    E --> K[CARD_MANAGER];
    E --> L[SETTINGS];

    subgraph Menu: MAIN_MENU
        F --> F1{Display Menu};
        F1 --> F2{Wait for Selection};
        F2 --> E;
    end

    subgraph Menu: READ_CARD
        G --> G1{Wait for Card};
        G1 --> G2{Read Data};
        G2 --> G3{Save to SD};
        G3 --> F;
    end

    subgraph Menu: WRITE_CARD
        H --> H1{Select File from SD};
        H1 --> H2{Wait for Card};
        H2 --> H3{Write Data};
        H3 --> F;
    end

    subgraph Menu: EMULATE_CARD
        I --> I1{Select File from SD};
        I1 --> I2{Emulate Card Data};
        I2 --> I3{Detect External Reader};
        I3 --> F;
    end

    subgraph Menu: BRUTE_FORCE
        J --> J1{Wait for Card};
        J1 --> J2{Try Common Keys};
        J2 --> J3{Save Found Keys};
        J3 --> F;
    end

    subgraph Menu: CARD_MANAGER
        K --> K1{List Saved Cards};
        K1 --> K2{Select Card};
        K2 --> K3{Delete Card};
        K3 --> F;
    end

    subgraph Menu: SETTINGS
        L --> L1{Display Settings};
        L1 --> L2{Modify Brightness/Debug};
        L2 --> F;
    end
