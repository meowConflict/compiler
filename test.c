void qqsort(int arr[6], int left, int len) {
    int pivot;
    int l , r;
    int tmp;
    if (len <= 1) {
        return;
    }
    pivot = arr[left];
    l = 1;
    r = len-1;
    while (l <= r) {
        while (l <= r && arr[left+l] <= pivot) {
            l=l+1;
        }
        if (l <= r) {
            tmp = arr[left+r];
            arr[left+r] = arr[left+l];
            arr[left+l] = tmp;
        } else {
            break;
        }
        while (l <= r && arr[left+r] >= pivot) {
            r=r-1;
        } 
        if (l <= r) {
            tmp = arr[left+r];
            arr[left+r] = arr[left+l];
            arr[left+l] = tmp;
        }
    }
    arr[left] = arr[left+r];
    arr[left+r] = pivot;
    qqsort(arr, left, r);
    qqsort(arr, left+r+1, len-r-1);
}
int main() {
    int n;
    int arr[100000],i;
    scanf("%d", n);
    for (i=0; i<n; i=i+1) {
        scanf("%d", arr+i);
    }
    qqsort(arr, 0, n);
    for (i=0; i<n; i=i+1) {
        printf("%d\n", arr[i]);
    }
}