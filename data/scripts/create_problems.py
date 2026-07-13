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
        aspect_ratio_limit=None,
        special_mode=None,  # None/"multiples"/"mixed_multiples"
        multiples_ratio=0.5
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

    mean_l = sum(length_range) / 2
    std_l = (length_range[1] - length_range[0]) / 6
    mean_w = sum(width_range) / 2
    std_w = (width_range[1] - width_range[0]) / 6

    valid_length_divisors = [d for d in range(length_range[0], length_range[1] + 1) if strip_width % d == 0]
    valid_width_divisors = [d for d in range(width_range[0], width_range[1] + 1) if strip_width % d == 0]

    if special_mode in ["multiples", "mixed_multiples"]:
        if not valid_length_divisors and not valid_width_divisors:
            raise ValueError(f"Brak dzielników szerokości {strip_width} w podanych zakresach!")

    while len(instances_data) < num_instances:
        current_dist = distribution_type
        if current_dist == "mixed":
            current_dist = random.choice(["uniform", "normal"])

        items = []
        for _ in range(num_item_types):
            while True:
                if current_dist == "normal":
                    l_rand = int(random.gauss(mean_l, std_l))
                    w_rand = int(random.gauss(mean_w, std_w))
                    l_rand = max(length_range[0], min(length_range[1], l_rand))
                    w_rand = max(width_range[0], min(width_range[1], w_rand))
                else:
                    l_rand = random.randint(length_range[0], length_range[1])
                    w_rand = random.randint(width_range[0], width_range[1])

                is_multiple = False
                if special_mode == "multiples":
                    is_multiple = True
                elif special_mode == "mixed_multiples":
                    is_multiple = (random.random() < multiples_ratio)

                required_demand = 0

                if is_multiple:
                    choices = []
                    if valid_length_divisors: choices.append('length')
                    if valid_width_divisors: choices.append('width')

                    div_choice = random.choice(choices)

                    if div_choice == 'length':
                        l = random.choice(valid_length_divisors)
                        w = w_rand
                        required_demand = strip_width // l
                    else:
                        l = l_rand
                        w = random.choice(valid_width_divisors)
                        required_demand = strip_width // w
                else:
                    l = l_rand
                    w = w_rand

                # Sprawdzamy czy kształt ma odpowiedni stosunek wysokości i szerokości
                if aspect_ratio_limit:
                    ratio = l / w
                    if not (aspect_ratio_limit[0] <= ratio <= aspect_ratio_limit[1]):
                        continue

                # Jeśli jest włączony tryb wielokrotności, ilość boxów musi być równa lub większa do zapełnienia szerokości stripa
                if is_multiple:
                    min_demand = max(demand_range[0], required_demand)
                    range_size = demand_range[1] - demand_range[0]
                    max_demand = max(demand_range[1], min_demand + range_size)

                    demand = random.randint(min_demand, max_demand)
                else:
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
        for inst in instances_data:
            f.write(f"{inst}\n")

    print(f"Wygenerowano plik '{filename}' z {num_instances} zadaniami.")


if __name__ == "__main__":
    # Zestaw 1
    # Losowy mix
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
    # Zestaw 2
    # Waskie prostokąty
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
    #     aspect_ratio_limit=(3.0, 10.0)
    # )
    # Zestaw 3
    # Kwadraty i prawie kwadraty
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
    #     aspect_ratio_limit=(0.8, 1.25)
    # )
    # Zestaw 4:
    # Wielokrotności boxów zawsze idealnie zapełniają szerokość stripa
    # generate_strip_packing_instances(
    #     filename="data/SPP_4.txt",
    #     num_instances=510000,
    #     strip_width=1000,
    #     strip_value=1000,
    #     num_item_types=10,
    #     length_range=(100, 500),
    #     width_range=(100, 500),
    #     demand_range=(1, 5),
    #     distribution_type="uniform",
    #     special_mode="multiples"
    # )
    # Zestaw 5:
    # Wielokrotności boxów czasem idealnie zapełniają szerokość stripa
    generate_strip_packing_instances(
        filename="data/SPP_5.txt",
        num_instances=510000,
        strip_width=1000,
        strip_value=1000,
        num_item_types=10,
        length_range=(100, 500),
        width_range=(100, 500),
        demand_range=(1, 5),
        distribution_type="uniform",
        special_mode="mixed_multiples",
        multiples_ratio=0.5
    )
