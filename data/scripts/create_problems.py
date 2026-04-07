import random


def generate_strip_packing_instances(
        filename="data/strip_packing_instances.txt",
        num_instances=5,
        strip_width=1000,
        strip_value=1000,
        num_item_types=10,
        length_range=(100, 400),
        width_range=(100, 400),
        demand_range=(1, 5),
        distribution_type="mixed",
        aspect_ratio_limit=None
):
    header = (
        "***2D Rectangular Problem***\n"
        "***Problem tests for the Open Dimension Problem (ODP/S)***\n"
        "Input parameter file: \n"
        "***********************************************************************\n"
        "Total number of instances \n"
        "Number of different large objects (j)\n"
        "LargeObject[j].Width\tLargeObject[j].Value\n"
        "Number of different item types (i)\n"
        "Item[i].Length\tItem[i].Width\tItem[i].Demand\n"
        "***********************************************************************\n"
    )
    instances_data = []

    # Przygotowanie parametrów dla rozkładu normalnego (jeśli wybrany)
    mean_l = sum(length_range) / 2
    std_l = (length_range[1] - length_range[0]) / 6
    mean_w = sum(width_range) / 2
    std_w = (width_range[1] - width_range[0]) / 6

    while len(instances_data) < num_instances:
        current_dist = distribution_type
        if current_dist == "mixed":
            current_dist = random.choice(["uniform", "normal"])

        # Generowanie boxow
        items = []
        for _ in range(num_item_types):
            while True:
                if current_dist == "normal":
                    l = int(random.gauss(mean_l, std_l))
                    w = int(random.gauss(mean_w, std_w))
                else:
                    l = random.randint(length_range[0], length_range[1])
                    w = random.randint(width_range[0], width_range[1])

                if aspect_ratio_limit:
                    ratio = l / w
                    if not (aspect_ratio_limit[0] <= ratio <= aspect_ratio_limit[1]):
                        continue

                demand = random.randint(demand_range[0], demand_range[1])
                items.append((l, w, demand))
                break

        instance_str = f"1\n{strip_width}\t{strip_value}\n{num_item_types}\n"
        for item in items:
            instance_str += f"{item[0]}\t{item[1]}\t{item[2]}\n"

        instances_data.append(instance_str.strip())

    with open(filename, 'w', encoding='utf-8') as f:
        f.write(header)
        f.write(f"{num_instances}\n")
        for i, inst in enumerate(instances_data):
            f.write(f"{inst}\n")

    print(f"Wygenerowano plik '{filename}' z {num_instances} zadaniami.")


if __name__ == "__main__":
    # Zestaw 1
    generate_strip_packing_instances(
        filename="data/SPP_1.txt",
        num_instances=510000,
        strip_width=1000,
        strip_value=1000,
        num_item_types=10,
        length_range=(100, 500),
        width_range=(100, 500),
        demand_range=(1, 5),
        distribution_type="uniform",
    )
    # # Zestaw 2
    # generate_strip_packing_instances(
    #     filename="data/SPP_2.txt",
    #     num_instances=510000,
    #     strip_width=1000,
    #     strip_value=1000,
    #     num_item_types=10,
    #     length_range=(100, 500),
    #     width_range=(100, 500),
    #     demand_range=(1, 5),
    #     distribution_type="uniform",
    #     aspect_ratio_limit=(3.0, 10.0) # waskie prostokąty
    # )
    # # Zestaw 3
    # generate_strip_packing_instances(
    #     filename="data/SPP_3.txt",
    #     num_instances=510000,
    #     strip_width=1000,
    #     strip_value=1000,
    #     num_item_types=10,
    #     length_range=(100, 500),
    #     width_range=(100, 500),
    #     demand_range=(1, 5),
    #     distribution_type="uniform",
    #     aspect_ratio_limit=(0.8, 1.25) # kwadraty
    # )
