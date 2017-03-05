#include <sys/queue.h>

#include <check.h>

struct q_int_elem {
	int elem;
	STAILQ_ENTRY(q_int_elem) _;
};
STAILQ_HEAD(q_int, q_int_elem);

START_TEST(test_fifo)
{
	struct q_int head;
	STAILQ_INIT(&head);
	ck_assert(STAILQ_EMPTY(&head));

	const struct q_int_elem *i = STAILQ_FIRST(&head);
	ck_assert_ptr_eq(NULL, i);

	struct q_int_elem a, b, c;
	a.elem = 3;
	b.elem = 2;
	c.elem = 5;
	STAILQ_INSERT_TAIL(&head, &a, _);
	STAILQ_INSERT_TAIL(&head, &b, _);
	STAILQ_INSERT_TAIL(&head, &c, _);

	ck_assert(!STAILQ_EMPTY(&head));

	i = STAILQ_FIRST(&head);
	ck_assert_int_eq(3, i->elem);
	i = STAILQ_NEXT(i, _);
	ck_assert_int_eq(2, i->elem);
	i = STAILQ_NEXT(i, _);
	ck_assert_int_eq(5, i->elem);
	ck_assert_ptr_eq(NULL, STAILQ_NEXT(i, _));

	STAILQ_REMOVE_HEAD(&head, _);
	ck_assert_int_eq(2, STAILQ_FIRST(&head)->elem);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_fifo);

	Suite *const suite = suite_create("stailq");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
